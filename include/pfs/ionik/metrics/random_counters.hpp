////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "system_counters.hpp"
#include "network_counters.hpp"
#include <pfs/ionik/error.hpp>
#include <pfs/ionik/exports.hpp>
#include <pfs/ionik/metrics/random_metrics_provider.hpp>

namespace ionik {
namespace metrics {

class random_system_counters
{
public:
    using counter_group = system_counters::counter_group;
    using metric_limits = random_default_provider::metric_limits;

private:
    random_default_provider _p;

public:
    IONIK__EXPORT random_system_counters ();
    IONIK__EXPORT random_system_counters (metric_limits && ml);
    IONIK__EXPORT ~random_system_counters () = default;
    IONIK__EXPORT random_system_counters (random_system_counters &&) = default;
    IONIK__EXPORT random_system_counters & operator = (random_system_counters &&) = default;

    random_system_counters (random_system_counters const &) = delete;
    random_system_counters & operator = (random_system_counters const &) = delete;

public:
    IONIK__EXPORT counter_group query (error * = nullptr);
    IONIK__EXPORT bool query (counter_group & counters, error * perr = nullptr);
};

class random_network_counters
{
public:
    using counter_group = network_counters::counter_group;
    using metric_limits = random_network_provider::metric_limits;

private:
    random_network_provider _p;

public:
    IONIK__EXPORT random_network_counters ();
    IONIK__EXPORT random_network_counters (metric_limits && ml);
    IONIK__EXPORT ~random_network_counters () = default;
    IONIK__EXPORT random_network_counters (random_network_counters &&) = default;
    IONIK__EXPORT random_network_counters & operator = (random_network_counters &&) = default;

    random_network_counters (random_network_counters const &) = delete;
    random_network_counters & operator = (random_network_counters const &) = delete;

public:
    IONIK__EXPORT counter_group query (error * = nullptr);
    IONIK__EXPORT bool query (counter_group & counters, error * perr = nullptr);
};

}} // namespace ionik::metrics
