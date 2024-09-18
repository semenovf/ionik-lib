////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/filesystem.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <minwindef.h>

namespace ionik {
namespace filesystem_monitor {
namespace backend {

struct notify_changes_entry
{
    pfs::filesystem::path dir_path;
    HANDLE waiting_handle {nullptr};
    int notify_filters {0};

    HANDLE hdir {nullptr};
    std::vector<char> buffer;
    std::unique_ptr<OVERLAPPED> overlapped_ptr;

    // Empty if monitoring directory content only.
    std::unordered_set<pfs::filesystem::path> child_filenames;
};

class win32
{
public:
    std::vector<HANDLE> waiting_handles;
    std::unordered_map<HANDLE, notify_changes_entry> watch_dirs;

    notify_changes_entry * locate_entry (pfs::filesystem::path const & dir_path);
    bool add_dir (pfs::filesystem::path const & path, error * perr);
    bool add_file (pfs::filesystem::path const & path, /*int notify_filter, */error * perr);

public: // static
    static bool read_dir_changes(notify_changes_entry & x);
};

}}} // namespace ionik::filesystem_monitor::backend
