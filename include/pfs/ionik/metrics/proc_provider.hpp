////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/ionik/error.hpp>
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
public:
    using string_view = pfs::string_view;

    struct record_view
    {
        string_view key;
        string_view value;
        string_view units;
    };

    struct record
    {
        std::string key;
        std::string value;
        std::string units;
    };

private:
    std::string _content;

private:
    bool read_all (error * perr);
    bool parse_record (std::string::const_iterator & pos, std::string::const_iterator last
        , record_view & rec, error * perr = nullptr);

public:
    proc_meminfo_provider ();

public:
    /**
     * Parses meminfo records into std::map.
     */
    std::map<string_view, record_view> parse_rv ( error * perr = nullptr);

    /**
     * Parses meminfo records into std::map.
     */
    std::map<std::string, record> parse ( error * perr = nullptr);

    template <typename F>
    bool query_rv (F && f, error * perr = nullptr)
    {
        if (!read_all(perr))
            return false;

        auto pos = _content.cbegin();
        auto last = _content.cend();

        record_view rec;

        while (parse_record(pos, last, rec))
            f(rec.key, rec.value, rec.units);

        return true;
    }

    template <typename F>
    bool query (F && f, error * perr = nullptr)
    {
        using pfs::to_string;

        if (!read_all(perr))
            return false;

        auto pos = _content.cbegin();
        auto last = _content.cend();

        record_view rec;

        while (parse_record(pos, last, rec))
            f(to_string(rec.key), to_string(rec.value), to_string(rec.units));

        return true;
    }
};

}} // namespace ionik::metrics
