////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/ionik/error.hpp>
#include <pfs/string_view.hpp>
#include <time.h>
#include <cstdint>
#include <vector>

namespace ionik {
namespace metrics {

// Provides CPU utilization by the current process based on `times(2)` call.

class process_times_provider
{
public:
    using string_view = pfs::string_view;

    struct cpu_core_info
    {
        std::string vendor_id;
        std::string model_name;
        counter_t   cache_size; // in bytes
    };

private:
    std::vector<cpu_core_info> _cpu_info;

    clock_t _recent_ticks {-1};
    clock_t _recent_sys {-1};
    clock_t _recent_usr {-1};

private:
    double calculate_cpu_usage (error * perr = nullptr);

public:
    process_times_provider (error * perr = nullptr);

public:
    /**
     * Supported keys:
     *      * cpu_usage - cpu utilization by the current process, in percents (double value);
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics

