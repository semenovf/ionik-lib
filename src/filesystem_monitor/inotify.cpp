////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.06.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "filesystem_monitor/monitor.hpp"
#include "filesystem_monitor/backend/inotify.hpp"
#include "filesystem_monitor/callbacks/emitter_callbacks.hpp"
#include "filesystem_monitor/callbacks/fptr_calbacks.hpp"
#include "filesystem_monitor/callbacks/functional.hpp"
#include <pfs/i18n.hpp>
#include <pfs/log.hpp>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/inotify.h>

namespace ionik {
namespace filesystem_monitor {

namespace fs = pfs::filesystem;
using rep_type = backend::inotify;

template <>
monitor<rep_type>::monitor (error * perr)
{
    _rep.fd = inotify_init1(IN_NONBLOCK);

    if (_rep.fd < 0) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , tr::_("inotify init failure")
            , pfs::system_error_text()
        });

        return;
    }

    _rep.ed = epoll_create1(0);

    if (_rep.ed < 0) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , tr::_("epoll create failure")
            , pfs::system_error_text()
        });

        return;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLERR | EPOLLET;
    ev.data.fd = _rep.fd;

    auto rc = epoll_ctl(_rep.ed, EPOLL_CTL_ADD, _rep.fd, & ev);

    if (rc < 0) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , tr::_("add entry to epoll failure")
            , pfs::system_error_text()
        });

        return;
    }
}

template <>
monitor<rep_type>::~monitor ()
{
    if (_rep.fd > 0) {
        for (auto & w: _rep.watch_map)
            inotify_rm_watch(_rep.fd, w.first);

        _rep.watch_map.clear();

        ::close(_rep.fd);
        _rep.fd = -1;
    }

    if (_rep.ed > 0) {
        ::close(_rep.ed);
        _rep.ed = -1;
    }
}

template <>
bool monitor<rep_type>::add (fs::path const & path, error * perr)
{
    if (!fs::exists(path)) {
        pfs::throw_or(perr, error {
              make_error_code(std::errc::invalid_argument)
            , tr::f_("attempt to watch non-existence path: {}", path)
        });

        return false;
    }

    auto canonical_path = fs::canonical(path);

    auto fd = inotify_add_watch(_rep.fd, fs::utf8_encode(canonical_path).c_str(), IN_ALL_EVENTS);

    if (fd < 0) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , tr::f_("add path to watching failure: {}", canonical_path)
            , pfs::system_error_text()
        });

        return false;
    }

    _rep.watch_map[fd] = std::move(canonical_path);
    return true;
}

template <>
template <typename Callbacks>
int monitor<rep_type>::poll (std::chrono::milliseconds timeout, Callbacks & cb, error * perr)
{
    struct epoll_event events[1];
    int rc = epoll_wait(_rep.ed, events, 1, timeout.count());

    if (rc == 0)
        return 0;

    if (rc < 0) {
        if (errno == EINTR) {
            // Is not a critical error, ignore it
        } else {
            pfs::throw_or(perr, error {
                  pfs::get_last_system_error()
                , tr::_("epoll wait failure")
                , pfs::system_error_text()
            });
        }

        return -1;
    }

    if (events[0].events & EPOLLERR) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , tr::_("error on inotify descriptor occurred while epolling")
        });

        return -1;
    }

    if (events[0].events & EPOLLIN) {
        char buffer [sizeof(struct inotify_event) + NAME_MAX + 1];
        ssize_t n = 0;

        while ((n = ::read(_rep.fd, buffer, sizeof(buffer))) > 0) {
            auto x = reinterpret_cast<inotify_event const *>(buffer);

            auto pos = _rep.watch_map.find(x->wd);

            if (pos == _rep.watch_map.end()) {
                pfs::throw_or(perr, error {
                      make_error_code(pfs::errc::unexpected_error)
                    , tr::f_("entry not found in watch map by descriptor: {}", x->wd)
                });

                return -1;
            }

            bool is_dir = x->mask & IN_ISDIR;

            fs::path path = pos->second;

            if (x->len > 0) {
                // Canonical function requires path existance
                //path = fs::canonical(path / fs::utf8_decode(x->name));
                path /= fs::utf8_decode(x->name);
            }

#if PFS__LOG_LEVEL >= 3
            std::string entry_type_str = is_dir ? "DIR_" : "FILE";
            std::string xname = x->len > 0 ? x->name : "<empty>";
#endif

            // File was accessed
            if (x->mask & IN_ACCESS) {
                LOG_TRACE_3("IN_ACCESS: {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.accessed)
                    cb.accessed(path);
            }

            // File was modified
            if (x->mask & IN_MODIFY) {
                LOG_TRACE_3("IN_MODIFY: {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.modified)
                    cb.modified(path);
            }

            // Metadata changed
            if (x->mask & IN_ATTRIB) {
                LOG_TRACE_3("IN_ATTRIB: {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.metadata_changed)
                    cb.metadata_changed(path);
            }

            // Opened
            if (x->mask & IN_OPEN) {
                LOG_TRACE_3("IN_OPEN  : {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.opened)
                    cb.opened(path);
            }

            // Closed
            if (x->mask & IN_CLOSE) {
                LOG_TRACE_3("IN_CLOSE : {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.closed)
                    cb.closed(path);
            }

            // Subfile was created
            if (x->mask & IN_CREATE) {
                LOG_TRACE_3("IN_CREATE: {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.created)
                    cb.created(path);
            }

            // Subfile was deleted
            if (x->mask & IN_DELETE) {
                LOG_TRACE_3("IN_DELETE: {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.deleted)
                    cb.deleted(path);
            }

            // Self was deleted
            if (x->mask & IN_DELETE_SELF) {
                LOG_TRACE_3("IN_DELETE_SELF: {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.deleted)
                    cb.deleted(path);
            }

            // Moves
            if (x->mask & IN_MOVE) {
                LOG_TRACE_3("IN_MOVE  : {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.moved)
                    cb.moved(path);
            }

            // Self was moved
            if (x->mask & IN_MOVE_SELF) {
                LOG_TRACE_3("IN_MOVE_SELF: {}: path={}; x->name={}", entry_type_str, path, xname);

                if (cb.moved)
                    cb.moved(path);
            }

            if (x->mask & IN_IGNORED) {
                LOG_TRACE_3("IN_IGNORED: {}: path={}; x->name={}", entry_type_str, path, xname);
            }
        }

        if (n < 0) {
            if (errno == EAGAIN) {
                ;
            } else {
                pfs::throw_or(perr, error {
                      pfs::get_last_system_error()
                    , tr::_("read inotify event failure")
                    , pfs::system_error_text()
                });

                return -1;
            }
        }
    }

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
