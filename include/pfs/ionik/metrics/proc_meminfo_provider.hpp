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
#include "counter.hpp"
#include <pfs/ionik/error.hpp>
#include <pfs/string_view.hpp>
#include <string>

namespace ionik {
namespace metrics {

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
     * If @a f return @c true the loop breaks.
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

}} // namespace ionik::metrics

