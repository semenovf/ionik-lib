////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
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
    std::string clone_content () const
    {
        return _content;
    }

    std::string move_content ()
    {
        return std::move(_content);
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
    bool parse_record (string_view::const_iterator & pos, string_view::const_iterator last
        , record_view & rec, error * perr = nullptr);

public:
    proc_meminfo_provider ();

public:
    /**
     * Сonsistently visits records with the function call @a f with a signature
     * bool (string_view const & key, string_view const & value, string_view const & units).
     * If @a f returna @c true the loop breaks.
     */
    template <typename F>
    bool query_all (F && f, error * perr = nullptr)
    {
        if (!read_all(perr))
            return false;

        string_view content_view {_content};
        auto pos = content_view.cbegin();
        auto last = content_view.cend();

        record_view rec;

        while (parse_record(pos, last, rec)) {
            if (f(rec.key, rec.value, rec.units))
                break;
        }

        return true;
    }

    /**
     * Supported keys:
     *      * MemTotal   - total amount of physical RAM, in bytes;
     *      * MemFree    - the amount of physical RAM left unused by the system, in bytes;
     *      * Cached     - the amount of physical RAM used as cache memory, in bytes;
     *      * SwapCached - the amount of swap used as cache memory, in bytes.
     *      * SwapTotal  - the total amount of swap available, in bytes.
     *      * SwapFree   - the total amount of swap free, in bytes.
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
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
    bool parse_record (string_view::const_iterator & pos, string_view::const_iterator last
        , record_view & rec, error * perr = nullptr);

public:
    proc_self_status_provider ();

public:
    /**
     * Сonsistently visits records with the function call @a f with a signature
     * bool (string_view const & key, std::vector<string_view> const & values).
     * If @a f returna @c true the loop breaks.
     */
    template <typename F>
    bool query_all (F && f, error * perr = nullptr)
    {
        if (!read_all(perr))
            return false;

        string_view content_view {_content};
        auto pos = content_view.cbegin();
        auto last = content_view.cend();

        record_view rec;

        while (parse_record(pos, last, rec)) {
            if (f(rec.key, rec.values))
                break;
        }

        return true;
    }

    /**
     * Supported keys:
     *      * VmSize - the virtual memory usage of the entire process, in bytes (it is the sum of VmLib, VmExe, VmData, and VmStk);
     *      * VmPeak - the peak virtual memory size, in bytes;
     *      * VmRSS  - resident set size, in bytes;
     *      * VmSwap - the swap memory used, in bytes.
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics
