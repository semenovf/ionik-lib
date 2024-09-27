////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version (proc_provider.hpp).
//      2024.09.27 Initial version (moved from proc_provider.hpp).
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/ionik/error.hpp>
#include <pfs/filesystem.hpp>
#include <string>

namespace ionik {
namespace metrics {

class proc_reader
{
    std::string _content;

public:
    proc_reader (pfs::filesystem::path const & path, error * perr = nullptr);

public:
    std::string clone_content () const
    {
        return _content;
    }

    std::string move_content ()
    {
        return std::move(_content);
    }
};

}} // namespace ionik::metrics
