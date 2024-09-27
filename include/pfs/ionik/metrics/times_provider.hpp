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
#include "pfs/ionik/error.hpp"
#include <pfs/optional.hpp>
#include <pfs/string_view.hpp>
#include <time.h>
#include <cstdint>
#include <vector>

#if _MSC_VER
#   include <minwindef.h>
#endif

namespace ionik {
namespace metrics {

// Provides CPU utilization by the current process based on `times(2)` call.

class times_provider
{
public:
    using string_view = pfs::string_view;

#if _MSC_VER
#else
    struct cpu_core_info
    {
        std::string vendor_id;
        std::string model_name;
        counter_t   cache_size; // in bytes
    };
#endif

private:
#if _MSC_VER
    std::uint32_t _core_count {0};
    ULARGE_INTEGER _recent_time;
    ULARGE_INTEGER _recent_sys;
    ULARGE_INTEGER _recent_usr;
#else
    std::vector<cpu_core_info> _cpu_info;

    clock_t _recent_ticks {-1};
    clock_t _recent_sys {-1};
    clock_t _recent_usr {-1};
#endif

private:
    /**
     * Calculates CPU usage.
     *
     * @return * @c nullopt - times() call failure;
     *         * -1.0       - if overflow, need to skip value;
     *         * > 0.0      - CPU usage in percents
     */
    pfs::optional<double> calculate_cpu_usage ();

public:
    times_provider (error * perr = nullptr);

public:
    /**
     * Supported keys:
     *      * cpu_usage - cpu utilization by the current process, in percents (double value);
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics

