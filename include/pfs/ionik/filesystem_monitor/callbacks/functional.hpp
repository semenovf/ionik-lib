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

    std::function<void (bool is_dir, path_t const &)> accessed;
    std::function<void (bool is_dir, path_t const &)> modified;
    std::function<void (bool is_dir, path_t const &)> metadata_changed;
    std::function<void (bool is_dir, path_t const &)> opened;
    std::function<void (bool is_dir, path_t const &)> closed;
    std::function<void (bool is_dir, path_t const &)> created;
    std::function<void (bool is_dir, path_t const &)> deleted;
    std::function<void (bool is_dir, path_t const &)> moved;
};

}} // namespace ionik::filesystem_monitor
