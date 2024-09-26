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
#include <pfs/string_view.hpp>
#include <memory>

namespace ionik {
namespace metrics {

class default_provider
{
    class impl;

public:
    using string_view = pfs::string_view;

private:
    std::unique_ptr<impl> _d;

public:
    default_provider (error * perr = nullptr);

public:
    /**
     * Supported keys:
     *      * cpu_usage       - CPU utilization by the current process, in percents (double value);
     *      * cpu_usage_total - total CPU utilization, in percents (double value);
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics
