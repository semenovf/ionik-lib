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
#include <pfs/emitter.hpp>

namespace ionik {
namespace filesystem_monitor {

struct emitter_callbacks
{
    using path_t = pfs::filesystem::path;

    pfs::emitter<bool /*is_dir*/, path_t const &> accessed;
    pfs::emitter<bool /*is_dir*/, path_t const &> modified;
    pfs::emitter<bool /*is_dir*/, path_t const &> metadata_changed;
    pfs::emitter<bool /*is_dir*/, path_t const &> opened;
    pfs::emitter<bool /*is_dir*/, path_t const &> closed;
    pfs::emitter<bool /*is_dir*/, path_t const &> created;
    pfs::emitter<bool /*is_dir*/, path_t const &> deleted;
    pfs::emitter<bool /*is_dir*/, path_t const &> moved;
};

struct emitter_mt_callbacks
{
    using path_t = pfs::filesystem::path;

    pfs::emitter_mt<bool /*is_dir*/, path_t const &> accessed;
    pfs::emitter_mt<bool /*is_dir*/, path_t const &> modified;
    pfs::emitter_mt<bool /*is_dir*/, path_t const &> metadata_changed;
    pfs::emitter_mt<bool /*is_dir*/, path_t const &> opened;
    pfs::emitter_mt<bool /*is_dir*/, path_t const &> closed;
    pfs::emitter_mt<bool /*is_dir*/, path_t const &> created;
    pfs::emitter_mt<bool /*is_dir*/, path_t const &> deleted;
    pfs::emitter_mt<bool /*is_dir*/, path_t const &> moved;
};

}} // namespace ionik::filesystem_monitor

