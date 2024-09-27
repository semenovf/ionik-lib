////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/ionik/error.hpp>
#include <pfs/optional.hpp>
#include <pfs/string_view.hpp>
#include <memory>

namespace ionik {
namespace metrics {

class default_counters
{
    class impl;

public:
    using string_view = pfs::string_view;

    struct counter_group
    {
        pfs::optional<double> cpu_usage_total;  // Total CPU usage, in percents
        pfs::optional<double> cpu_usage;        // Current process CPU usage, in percents

        pfs::optional<std::int64_t> ram_total;  // Total RAM, in bytes
        pfs::optional<std::int64_t> ram_free;   // Available RAM, in bytes
        pfs::optional<double> ram_usage_total;  // RAM used, in percents
        pfs::optional<std::int64_t> swap_total; // Total swap space size, in bytes
        pfs::optional<std::int64_t> swap_free;  // Swap space still available, in bytes
        pfs::optional<double> swap_usage_total; // Swap used, in percents

        pfs::optional<std::int64_t> mem_usage;      // Current process memory usage, in bytes
        pfs::optional<std::int64_t> mem_peak_usage; // Peak memory usage by the current process, in bytes
        pfs::optional<std::int64_t> swap_usage;     // Current process swap usage, in bytes
    };

private:
    std::unique_ptr<impl> _d;

public:
    default_counters (error * perr = nullptr);
    ~default_counters ();

public:
    counter_group query (error * perr = nullptr);
};

}} // namespace ionik::metrics
