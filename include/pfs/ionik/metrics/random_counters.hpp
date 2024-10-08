////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "default_counters.hpp"
#include <pfs/ionik/error.hpp>
#include <pfs/ionik/exports.hpp>
#include <pfs/ionik/metrics/random_metrics_provider.hpp>

namespace ionik {
namespace metrics {

class random_counters
{
public:
    using counter_group = default_counters::counter_group;
    using metric_limits = random_metrics_provider::metric_limits;

private:
    random_metrics_provider _rmp;

public:
    IONIK__EXPORT random_counters ();
    IONIK__EXPORT random_counters (metric_limits && ml);
    IONIK__EXPORT ~random_counters () = default;
    IONIK__EXPORT random_counters (random_counters &&) = default;
    IONIK__EXPORT random_counters & operator = (random_counters &&) = default;

    random_counters (random_counters const &) = delete;
    random_counters & operator = (random_counters const &) = delete;

public:
    IONIK__EXPORT counter_group query (error *);
};

}} // namespace ionik::metrics
