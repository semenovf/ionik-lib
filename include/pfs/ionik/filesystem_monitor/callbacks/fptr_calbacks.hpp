////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.07.01 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/filesystem.hpp>

namespace ionik {
namespace filesystem_monitor {

struct fptr_callbacks
{
    using path_t = pfs::filesystem::path;

    void (* accessed) (bool is_dir, path_t const &);
    void (* modified) (bool is_dir, path_t const &);
    void (* metadata_changed) (bool is_dir, path_t const &);
    void (* opened) (bool is_dir, path_t const &);
    void (* closed) (bool is_dir, path_t const &);
    void (* created) (bool is_dir, path_t const &);
    void (* deleted) (bool is_dir, path_t const &);
    void (* moved) (bool is_dir, path_t const &);
};

}} // namespace ionik::filesystem_monitor

