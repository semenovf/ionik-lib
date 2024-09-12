////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "ionik/error.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/string_view.hpp>
#include <map>
#include <string>

namespace ionik {
namespace metrics {

class proc_reader
{
    std::string _content;

public:
    proc_reader (pfs::filesystem::path const & path, error * perr = nullptr);

public:
    std::string content () const noexcept
    {
        return _content;
    }
};

class proc_stat_provider
{

};

class proc_meminfo_provider
{
private:
    using string_view = pfs::string_view;

private:
    std::string _content;
    std::map<string_view, string_view> _mapping;

private:
    bool parse_key_value (std::string::const_iterator & pos, std::string::const_iterator last);
    bool parse (error * perr = nullptr);

public:
    proc_meminfo_provider ();

public:
    bool query (error * perr = nullptr);
};

}} // namespace ionik::metrics
