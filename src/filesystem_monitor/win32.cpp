////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "filesystem_monitor/monitor.hpp"
#include "filesystem_monitor/backend/win32.hpp"
#include "filesystem_monitor/callbacks/emitter_callbacks.hpp"
#include "filesystem_monitor/callbacks/fptr_calbacks.hpp"
#include "filesystem_monitor/callbacks/functional.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <pfs/log.hpp>
#include <pfs/memory.hpp>
#include <pfs/windows.hpp>
#include <algorithm>
#include <fileapi.h>

// References.
// 1.1. [FindFirstChangeNotificationW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstchangenotificationw)
// 1.2. [FindNextChangeNotification](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findnextchangenotification)
// 1.3. [FindCloseChangeNotification](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findclosechangenotification)
// 1.4. [WaitForMultipleObjects](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects)
// 1.5. [Obtaining Directory Change Notifications](https://learn.microsoft.com/en-us/windows/win32/fileio/obtaining-directory-change-notifications)

// 2.1. [ReadDirectoryChangesW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw)
// 2.2. [Understanding ReadDirectoryChangesW - Part 1](https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html)
// 2.3. [Understanding ReadDirectoryChangesW - Part 2](https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw_19.html)
// 2.4. [Example of how to poll ReadDirectoryChangesW on Windows](https://gist.github.com/nickav/a57009d4fcc3b527ed0f5c9cf30618f8)

namespace fs = pfs::filesystem;

constexpr std::size_t k_result_buffer_size = 16384;

namespace ionik {
namespace filesystem_monitor {

namespace backend {

inline bool win32::read_dir_changes (notify_changes_entry & x)
{
    auto success = ReadDirectoryChangesW(x.hdir
        , x.buffer.data()
        , x.buffer.size()
        , FALSE
        , x.notify_filters
        , nullptr
        , & *x.overlapped_ptr
        , nullptr) != 0;

    return success;
}

notify_changes_entry * win32::locate_entry (pfs::filesystem::path const & dir_path)
{
    auto pos = std::find_if(watch_dirs.begin(), watch_dirs.end(), [& dir_path] (std::unordered_map<HANDLE, notify_changes_entry>::value_type const & pair) {
        return pair.second.dir_path == dir_path;
    });

    if (pos == watch_dirs.end())
        return nullptr;

    return & pos->second;
}

bool win32::add_dir (fs::path const & path, error* perr)
{
    if (!fs::exists(path)) {
        pfs::throw_or(perr, error {
              make_error_code(std::errc::invalid_argument)
            , tr::f_("attempt to watch non-existence path: {}", path)
        });

        return false;
    }

    auto canonical_path = fs::canonical(path);

    // Already watching
    if (locate_entry(canonical_path) != nullptr)
        return true;

    DWORD share_mode = FILE_SHARE_READ
        | FILE_SHARE_WRITE
        | FILE_SHARE_DELETE;

    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS
        | FILE_FLAG_OVERLAPPED;

    HANDLE hdir = ::CreateFileW(canonical_path.c_str()
        , FILE_LIST_DIRECTORY // ReadDirectoryChangesW requirement (can be GENERIC_READ)
        , share_mode
        , nullptr  // security descriptor
        , OPEN_EXISTING
        , flags
        , nullptr);

    if (hdir == INVALID_HANDLE_VALUE) {
        pfs::throw_or(perr, error{
              errc::system_error
            , tr::f_("add path to watching failure: {}", canonical_path)
            , pfs::system_error_text(errno)
        });

        return false;
    }

    HANDLE waiting_handle = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (waiting_handle == nullptr) {
        pfs::throw_or(perr, error {
              errc::system_error
            , tr::f_("add path to watching failure: {}", canonical_path)
            , pfs::system_error_text(errno)
        });

        return false;
    }

    backend::notify_changes_entry x;

    x.dir_path = canonical_path;
    x.hdir = hdir;
    x.notify_filters = FILE_NOTIFY_CHANGE_FILE_NAME
        | FILE_NOTIFY_CHANGE_DIR_NAME
        | FILE_NOTIFY_CHANGE_ATTRIBUTES
        | FILE_NOTIFY_CHANGE_SIZE
        | FILE_NOTIFY_CHANGE_LAST_WRITE
        | FILE_NOTIFY_CHANGE_LAST_ACCESS
        | FILE_NOTIFY_CHANGE_CREATION
        | FILE_NOTIFY_CHANGE_SECURITY;

    x.buffer.resize(k_result_buffer_size);

    x.waiting_handle = waiting_handle;
    x.overlapped_ptr = pfs::make_unique<OVERLAPPED>();
    x.overlapped_ptr->hEvent = waiting_handle;

    auto success = read_dir_changes(x);

    if (!success) {
        pfs::throw_or(perr, error{
              errc::system_error
            , tr::f_("add path to watching failure: {}", canonical_path)
            , pfs::system_error_text(errno)
        });

        return false;
    }

    waiting_handles.push_back(waiting_handle);
    watch_dirs[waiting_handle] = std::move(x);

    return success;
}

bool win32::add_file (fs::path const & path, error * perr)
{
    if (!fs::exists(path)) {
        pfs::throw_or(perr, error {
              make_error_code(std::errc::invalid_argument)
            , tr::f_("attempt to watch non-existence path: {}", path)
        });

        return false;
    }

    auto canonical_path = fs::canonical(path);
    auto parent_path = fs::canonical(canonical_path.parent_path());
    auto filename = canonical_path.filename();

    //LOGD("~~~", "PARENT PATH: {}", parent_path);
    //LOGD("~~~", "PATH       : {}", canonical_path);
    //LOGD("~~~", "FILENAME   : {}", filename);

    auto success = add_dir(parent_path, perr);

    if (!success)
        return false;

    auto * xptr = locate_entry(parent_path);

    PFS__TERMINATE(xptr != nullptr, "Unexpected error");

    auto pos = xptr->child_filenames.find(filename);

    // Already monitored
    if (pos != xptr->child_filenames.end())
        return true;

    xptr->child_filenames.insert(std::move(filename));

    return true;
}

} // namespace backend

using rep_type = backend::win32;

template <>
monitor<rep_type>::monitor (error * perr)
{}

template <>
monitor<rep_type>::~monitor ()
{
    for (auto & item: _rep.watch_dirs) {
        auto & x = item.second;

        if (x.waiting_handle != nullptr) {
            CloseHandle(x.waiting_handle);
            x.waiting_handle = nullptr;
        }

        if (x.hdir != nullptr) {
            CloseHandle(x.hdir);
            x.hdir = nullptr;
        }
    }
}

template <>
bool monitor<rep_type>::add (fs::path const & path, error * perr)
{
    bool success = true;

    // Monitor directory content
    if (fs::is_directory(path))
        success = success && _rep.add_dir(path, perr);

    // Monitor file and directory itself
    success = success && _rep.add_file(path, perr);

    return success;
}

template <>
template <typename Callbacks>
int monitor<rep_type>::poll (std::chrono::milliseconds timeout, Callbacks & cb, error * perr)
{
    if (_rep.waiting_handles.empty())
        return 0;

    bool has_removed = false;
    auto count = static_cast<DWORD>(_rep.waiting_handles.size());
    auto rc = WaitForMultipleObjects(static_cast<DWORD>(count), _rep.waiting_handles.data(), FALSE
        , static_cast<DWORD>(timeout.count()));

    if (rc == WAIT_TIMEOUT)
        return 0;
       
    if (rc == WAIT_FAILED) {
        pfs::throw_or(perr, error { errc::system_error
            , tr::_("WaitForMultipleObjects failure")
            , pfs::system_error_text()
        });

        return -1;
    }

    if (rc >= WAIT_OBJECT_0 && rc < WAIT_OBJECT_0 + count) {
        int index = rc - WAIT_OBJECT_0;

        HANDLE fd = _rep.waiting_handles[index];
        auto pos = _rep.watch_dirs.find(fd);

        if (pos == _rep.watch_dirs.end()) {
            pfs::throw_or(perr, error { pfs::errc::unexpected_error
                , tr::_("watch entity not found")
            });

            return -1;
        }

        auto & x = pos->second;
        DWORD bytes_transferred = 0;
        DWORD offset = 0;
        FILE_NOTIFY_INFORMATION const * file_info_ptr = nullptr;

        BOOL wait_flag = TRUE;
        auto rc = ::GetOverlappedResult(x.hdir, & *x.overlapped_ptr, & bytes_transferred, wait_flag);

        do {
            char const * buffer = x.buffer.data();
            file_info_ptr = reinterpret_cast<FILE_NOTIFY_INFORMATION const *>(buffer + offset);
            DWORD nwchars = file_info_ptr->FileNameLength / sizeof(wchar_t);
            //auto path = pfs::windows::utf8_encode(file_info_ptr->FileName, nwchars);
            fs::path filename {file_info_ptr->FileName, file_info_ptr->FileName + nwchars};

            offset += file_info_ptr->NextEntryOffset;

            auto is_path_monitored = false;

            if (x.child_filenames.empty()) {
                is_path_monitored = true;
            } else {
                is_path_monitored = x.child_filenames.find(filename) != x.child_filenames.end();
            }

            if (!is_path_monitored)
                continue;

            switch (file_info_ptr->Action) {
                case FILE_ACTION_ADDED:
                    LOGD("~~~", "FILE_ACTION_ADDED: {}", filename);

                    if (cb.created)
                        cb.created(x.dir_path / filename);

                    break;

                case FILE_ACTION_REMOVED:
                    LOGD("~~~", "FILE_ACTION_REMOVED: {}", filename);

                    if (cb.deleted)
                        cb.deleted(x.dir_path / filename);

                    break;

                case FILE_ACTION_MODIFIED:
                    LOGD("~~~", "FILE_ACTION_MODIFIED: {}", filename);

                    if (cb.modified)
                        cb.modified(x.dir_path / filename);

                    break;

                case FILE_ACTION_RENAMED_OLD_NAME:
                    LOGD("~~~", "FILE_ACTION_RENAMED_OLD_NAME: {}", filename);

                    if (cb.moved)
                        cb.moved(x.dir_path / filename);

                    break;

                case FILE_ACTION_RENAMED_NEW_NAME:
                    LOGD("~~~", "FILE_ACTION_RENAMED_NEW_NAME: {}", filename);

                    if (cb.moved)
                        cb.moved(x.dir_path / filename);

                    break;

                default:
                    LOGD("~~~", "FILE_ACTION_??? ({}): {}", file_info_ptr->Action, filename);
                    break;
            }
        } while (file_info_ptr != nullptr && file_info_ptr->NextEntryOffset != 0);

        ::ResetEvent(x.overlapped_ptr->hEvent);

        if (!backend::win32::read_dir_changes(x)) {
            pfs::throw_or(perr, error {pfs::errc::unexpected_error
                , tr::_("ReadDirectoryChangesW failure")
            });

            return -1;
        }
    }

    //if (has_removed) {
    //    for (auto pos = _rep.waiting_handles.begin(), last = _rep.handles.end(); pos != last; ) {
    //        if (*pos == INVALID_HANDLE_VALUE)
    //            pos = _rep.handles.erase(pos);
    //        else
    //            ++pos;
    //    }
    //}

//    if (events[0].events & EPOLLIN) {
//        char buffer[sizeof(struct inotify_event) + NAME_MAX + 1];
//        ssize_t n = 0;
//
//        while ((n = ::read(_rep.fd, buffer, sizeof(buffer))) > 0) {
//            auto x = reinterpret_cast<inotify_event const*>(buffer);
//
//            auto pos = _rep.watch_map.find(x->wd);
//
//            if (pos == _rep.watch_map.end()) {
//                pfs::throw_or(perr, error{
//                        make_error_code(pfs::errc::unexpected_error)
//                    , tr::f_("entry not found in watch map by descriptor: {}", x->wd)
//                    });
//
//                return -1;
//            }
//
//            bool is_dir = x->mask & IN_ISDIR;
//
//            fs::path path = pos->second;
//
//            if (x->len > 0) {
//                // Canonical function requires path existance
//                //path = fs::canonical(path / fs::utf8_decode(x->name));
//                path /= fs::utf8_decode(x->name);
//            }
//
//#if PFS__LOG_LEVEL >= 3
//            std::string entry_type_str = is_dir ? "DIR_" : "FILE";
//            std::string xname = x->len > 0 ? x->name : "<empty>";
//#endif
//
//            // File was accessed
//            if (x->mask & IN_ACCESS) {
//                LOG_TRACE_3("IN_ACCESS: {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.accessed)
//                    cb.accessed(is_dir, path);
//            }
//
//            // File was modified
//            if (x->mask & IN_MODIFY) {
//                LOG_TRACE_3("IN_MODIFY: {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.modified)
//                    cb.modified(is_dir, path);
//            }
//
//            // Metadata changed
//            if (x->mask & IN_ATTRIB) {
//                LOG_TRACE_3("IN_ATTRIB: {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.metadata_changed)
//                    cb.metadata_changed(is_dir, path);
//            }
//
//            // Opened
//            if (x->mask & IN_OPEN) {
//                LOG_TRACE_3("IN_OPEN  : {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.opened)
//                    cb.opened(is_dir, path);
//            }
//
//            // Closed
//            if (x->mask & IN_CLOSE) {
//                LOG_TRACE_3("IN_CLOSE : {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.closed)
//                    cb.closed(is_dir, path);
//            }
//
//            // Subfile was created
//            if (x->mask & IN_CREATE) {
//                LOG_TRACE_3("IN_CREATE: {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.created)
//                    cb.created(is_dir, path);
//            }
//
//            // Subfile was deleted
//            if (x->mask & IN_DELETE) {
//                LOG_TRACE_3("IN_DELETE: {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.deleted)
//                    cb.deleted(is_dir, path);
//            }
//
//            // Self was deleted
//            if (x->mask & IN_DELETE_SELF) {
//                LOG_TRACE_3("IN_DELETE_SELF: {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.deleted)
//                    cb.deleted(is_dir, path);
//            }
//
//            // Moves
//            if (x->mask & IN_MOVE) {
//                LOG_TRACE_3("IN_MOVE  : {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.moved)
//                    cb.moved(is_dir, path);
//            }
//
//            // Self was moved
//            if (x->mask & IN_MOVE_SELF) {
//                LOG_TRACE_3("IN_MOVE_SELF: {}: path={}; x->name={}", entry_type_str, path, xname);
//
//                if (cb.moved)
//                    cb.moved(is_dir, path);
//            }
//
//            if (x->mask & IN_IGNORED) {
//                LOG_TRACE_3("IN_IGNORED: {}: path={}; x->name={}", entry_type_str, path, xname);
//            }
//        }
//
//        if (n < 0) {
//            if (errno == EAGAIN) {
//                ;
//            }
//            else {
//                pfs::throw_or(perr, error{
//                        errc::system_error
//                    , tr::_("read inotify event failure")
//                    , pfs::system_error_text()
//                    });
//
//                return -1;
//            }
//        }
//    }

    return rc;
}

template
int monitor<rep_type>::poll<functional_callbacks> (std::chrono::milliseconds timeout, functional_callbacks & cb, error * perr);

template
int monitor<rep_type>::poll<fptr_callbacks> (std::chrono::milliseconds timeout, fptr_callbacks & cb, error * perr);

template
int monitor<rep_type>::poll<emitter_callbacks> (std::chrono::milliseconds timeout, emitter_callbacks & cb, error * perr);

template
int monitor<rep_type>::poll<emitter_mt_callbacks> (std::chrono::milliseconds timeout, emitter_mt_callbacks & cb, error * perr);

}} // namespace ionik::filesystem_monitor
