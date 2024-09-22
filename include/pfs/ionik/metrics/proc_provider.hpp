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
#include <vector>

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
    std::map<string_view, record_view> parse ( error * perr = nullptr);

    /**
     * Сonsistently visits records with the function call @a f with a signature
     * bool (string_view const & key, string_view const & value, string_view const & units).
     * If @a f returna @c true the loop breaks.
     */
    template <typename F>
    bool query (F && f, error * perr = nullptr)
    {
        if (!read_all(perr))
            return false;

        auto pos = _content.cbegin();
        auto last = _content.cend();

        record_view rec;

        while (parse_record(pos, last, rec)) {
            if (f(rec.key, rec.value, rec.units))
                break;
        }

        return true;
    }
};

class proc_self_status_provider
{
public:
    using string_view = pfs::string_view;

    struct record_view
    {
        string_view key;
        std::vector<string_view> values;
    };

private:
    std::string _content;

private:
    bool read_all (error * perr);
    bool parse_record (std::string::const_iterator & pos, std::string::const_iterator last
        , record_view & rec, error * perr = nullptr);

public:
    proc_self_status_provider ();

public:
    /**
     * Parses meminfo records into std::map.
     */
    std::map<string_view, record_view> parse ( error * perr = nullptr);

    /**
     * Сonsistently visits records with the function call @a f with a signature
     * bool (string_view const & key, std::vector<string_view> const & values).
     * If @a f returna @c true the loop breaks.
     */
    template <typename F>
    bool query (F && f, error * perr = nullptr)
    {
        if (!read_all(perr))
            return false;

        auto pos = _content.cbegin();
        auto last = _content.cend();

        record_view rec;

        while (parse_record(pos, last, rec)) {
            if (f(rec.key, rec.values))
                break;
        }

        return true;
    }
};

}} // namespace ionik::metrics
