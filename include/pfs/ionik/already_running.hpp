////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.12.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "error.hpp"
#include "exports.hpp"
#include "pfs/bits/operationsystem.h"
#include <string>

#if defined(PFS__OS_LINUX)
#   include "pfs/filesystem.hpp"
#elif defined(PFS__OS_WIN)
#   include <windows.h>
#else
#   error "Unsupported operation system yet"
#endif

namespace ionik {

class already_running
{
#if defined(PFS__OS_LINUX)
    int _fd {-1};
    pfs::filesystem::path _lock_file_path;

#elif defined(PFS__OS_WIN)
    HANDLE _mutex {nullptr};
#endif

public:
    /**
     * @param unique_name For Linux value must be a valid filename (see open()), for Windows value
     *        must be a valid name for the named mutex (see CreateMutex())
     */
    already_running (std::string const & unique_name, error * perr = nullptr);

    already_running () = delete;
    already_running (already_running const &) = delete;
    already_running (already_running &&) = delete;
    already_running & operator = (already_running const &) = delete;
    already_running & operator = (already_running &&) = delete;

    ~already_running ();

    bool operator () () const noexcept;
};

} // namespace ionik
