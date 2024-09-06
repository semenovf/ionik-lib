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

    void (* accessed) (path_t const &);
    void (* modified) (path_t const &);
    void (* metadata_changed) (path_t const &);
    void (* opened) (path_t const &);
    void (* closed) (path_t const &);
    void (* created) (path_t const &);
    void (* deleted) (path_t const &);
    void (* moved) (path_t const &);
};

}} // namespace ionik::filesystem_monitor

