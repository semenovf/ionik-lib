////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/ionik/error.hpp>
#include <pfs/ionik/exports.hpp>
#include <pfs/string_view.hpp>
#include <cstdint>
#include <string>
#include <utility>

namespace ionik {
namespace metrics {

class random_metrics_provider
{
public:
    struct metric_limits
    {
        int precision {2};
        std::pair<int, int> cpu_usage_total_range {15, 20}; // percents (min, max)
        std::pair<int, int> cpu_usage_range {5, 10};        // percents (min, max)
        std::int64_t ram_total {16L * 1024 * 1024 * 1024};  // absolute value (16 GiB)
        std::pair<int, int> ram_free_range {90, 95};        // percents (min, max)
        std::int64_t swap_total {2L * 1024 * 1024 * 1024};  // absolute value (2 GiB)
        std::pair<int, int> swap_free_range {98, 100};      // percents (min, max)
        std::pair<int, int> mem_usage {3, 5};               // percents (min, max)
    };

private:
    metric_limits _ml;

public:
    using string_view = pfs::string_view;

public:
    IONIK__EXPORT random_metrics_provider ();
    IONIK__EXPORT random_metrics_provider (metric_limits && ml);

public:
    /**
     * Supported keys (see default counters::counter_group):
     *      * cpu_usage_total  - total CPU usage, in percents (double);
     *      * cpu_usage        - current process CPU usage, in percents (double);
     *      * ram_total        - total RAM, in bytes (std::int64_t);
     *      * ram_free         - available RAM, in bytes (std::int64_t);
     *      * swap_total       - total swap space size, in bytes (std::int64_t);
     *      * swap_free        - swap space still available, in bytes (std::int64_t);
     *      * mem_usage        - current process memory usage, in bytes (std::int64_t);
     */
    IONIK__EXPORT bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics


