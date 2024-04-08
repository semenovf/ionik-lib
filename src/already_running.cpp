////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.12.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/already_running.hpp"

#if defined(PFS__OS_LINUX)
#   include "pfs/standard_paths.hpp"
#   include <sys/file.h>
#   include <unistd.h>

namespace fs = pfs::filesystem;

#elif defined(PFS__OS_WIN)
#   include "pfs/windows.hpp"
#else
#   error "Unsupported operation system yet"
#endif

namespace ionik {

already_running::already_running (std::string const & unique_name, error * perr)
{
#if defined(PFS__OS_LINUX)

    _lock_file_path = fs::standard_paths::temp_folder() / fs::utf8_decode(unique_name + ".lock");
    _fd = open(fs::utf8_encode(_lock_file_path).c_str(), O_WRONLY | O_CREAT, 0600);

    if (_fd < 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), pfs::system_error_text());
        return;
    }

    auto rc = flock(_fd, LOCK_EX | LOCK_NB);

    if (rc < 0) {
        ::close(_fd);
        _fd = -1;
    }

#elif defined(PFS__OS_WIN)

    SetLastError(0);

    _mutex = CreateMutex(NULL, TRUE, pfs::windows::utf8_decode(unique_name.c_str()).c_str());

    auto last_error = GetLastError();

    if (last_error != 0) {
        if (_mutex != nullptr) {
            CloseHandle(_mutex);
            _mutex = nullptr;
        }

        if (last_error != ERROR_ALREADY_EXISTS) {
            pfs::throw_or(perr, pfs::get_last_system_error(), pfs::system_error_text());
            return;
        }
    }

#endif
}

already_running::~already_running ()
{
#if defined(PFS__OS_LINUX)
    if (_fd > 0) {
        flock(_fd, LOCK_UN);
        ::close(_fd);
        _fd = -1;
        fs::remove(_lock_file_path);
        _lock_file_path.clear();
    }

#elif defined(PFS__OS_WIN)
    if (_mutex != nullptr) {
        CloseHandle(_mutex);
        _mutex = nullptr;
    }
#endif
}

bool already_running::operator () () const noexcept
{
#if defined(PFS__OS_LINUX)
    return _fd < 0;
#elif defined(PFS__OS_WIN)
    return _mutex == nullptr;
#endif
}

} // namespace ionik
