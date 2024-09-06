////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.06.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/filesystem.hpp>
#include <functional>

namespace ionik {
namespace filesystem_monitor {

struct functional_callbacks
{
    using path_t = pfs::filesystem::path;

    std::function<void (path_t const &)> accessed;         // inotify
    std::function<void (path_t const &)> modified;         // inotify, win32
    std::function<void (path_t const &)> metadata_changed; // inotify
    std::function<void (path_t const &)> opened;           // inotify
    std::function<void (path_t const &)> closed;           // inotify
    std::function<void (path_t const &)> created;          // inotify, win32
    std::function<void (path_t const &)> deleted;          // inotify, win32
    std::function<void (path_t const &)> moved;            // inotify, win32
};

}} // namespace ionik::filesystem_monitor
