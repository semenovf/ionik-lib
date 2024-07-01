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

    std::function<void (path_t const &)> accessed;
    std::function<void (path_t const &)> metadata_changed;
    std::function<void (path_t const &)> opened;
    std::function<void (path_t const &)> closed;
    std::function<void (path_t const &)> deleted;
};

}} // namespace ionik::filesystem_monitor
