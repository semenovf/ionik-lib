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
#include <unordered_map>

namespace ionik {
namespace filesystem_monitor {
namespace backend {

struct inotify
{
    // File descriptor referring to the inotify instance.
    int fd;

    // epoll descriptor
    int ed;

    std::unordered_map<int, pfs::filesystem::path> watch_map;
};

}}} // namespace ionik::filesystem_monitor::backend
