////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/optional.hpp>
#include <pfs/string_view.hpp>
#include <pfs/ionik/error.hpp>
#include <cstdlib>
#include <string>
#include <vector>

namespace ionik {
namespace metrics {

class proc_stat_provider
{
public:
    using string_view = pfs::string_view;

    struct record_view
    {
        string_view key;
        std::vector<string_view> values;
    };

private:
    struct cpu_data
    {
        std::int64_t usr {0};
        std::int64_t usr_low {0};
        std::int64_t sys {0};
        std::int64_t idl {0};
    };

private:
    std::string _content;
    std::vector<cpu_data> _cpu_recent_data;

private:
    bool read_all (error * perr);
    bool parse_record (string_view::const_iterator & pos, string_view::const_iterator last
        , record_view & rec, error * perr);
    pfs::optional<cpu_data> parse_cpu_data (record_view const & rec);

    /**
     * Calculates CPU usage.
     *
     * @return * @c nullopt - on failure;
     *         * -1.0       - if overflow, need to skip value;
     *         * > 0.0      - CPU usage in percents
     */
    pfs::optional<double> calculate_cpu_usage (record_view const & rec);

public:
    proc_stat_provider (error * perr = nullptr);

public:
    /**
     * Сonsistently visits records with the function call @a f with a signature
     * bool (string_view const & key, std::vector<string_view> const & values).
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

        while (parse_record(pos, last, rec, perr)) {
            if (f(rec.key, rec.values))
                break;
        }

        return true;
    }

    /**
     * Supported keys:
     *      * cpu  - total CPU utilization, in percents (double value);
     *      * cpuX - CPU utilization for the core with X index, in percents (double value);
     *
     * @return @c false on failure.
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics

