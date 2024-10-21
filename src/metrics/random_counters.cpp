////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/counter.hpp"
#include "ionik/metrics/random_counters.hpp"
#include <pfs/string_view.hpp>

namespace ionik {
namespace metrics {

using string_view = pfs::string_view;

random_counters::random_counters ()
    : random_counters(metric_limits{})
{}

random_counters::random_counters (metric_limits && ml)
    : _rmp(std::move(ml))
{}

random_counters::counter_group
random_counters::query (error *)
{
    counter_group counters;

    auto success = _rmp.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
        if (key == "cpu_usage_total") {
            static_cast<counter_group *>(pcounters)->cpu_usage_total = to_double(value);
        } else if (key == "cpu_usage") {
            static_cast<counter_group *>(pcounters)->cpu_usage = to_double(value);
        } else if (key == "ram_total") {
            static_cast<counter_group *>(pcounters)->ram_total = to_integer(value);
        } else if (key == "ram_free") {
            static_cast<counter_group *>(pcounters)->ram_free = to_integer(value);
        } else if (key == "swap_total") {
            static_cast<counter_group *>(pcounters)->swap_total = to_integer(value);
        } else if (key == "swap_free") {
            static_cast<counter_group *>(pcounters)->swap_free = to_integer(value);
        } else if (key == "mem_usage") {
            static_cast<counter_group *>(pcounters)->mem_usage = to_integer(value);
        }

        return false;
    }, & counters);


    if (success) {
        if (counters.ram_total && counters.ram_free) {
            counters.ram_usage_total = ((static_cast<double>(*counters.ram_total) - *counters.ram_free)
                / *counters.ram_total) * 100.0;
        }

        if (counters.swap_total && counters.swap_free) {
            counters.swap_usage_total = ((static_cast<double>(*counters.swap_total) - *counters.swap_free)
                / *counters.swap_total) * 100.0;
        }

        return counters;
    }

    return counter_group{};
}

std::vector<random_counters::net_counter_group>
random_counters::query_net_counters (error *)
{
    std::vector<net_counter_group> result;
    result.resize(1);

    auto & x = result[0];
    x.iface = "eth0";
    x.readable_name = x.iface;

    _rmp.query_net_counters([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
        if (key == "rx_bytes") {
            static_cast<net_counter_group *>(pcounters)->rx_bytes = to_integer(value);
        } else if (key == "tx_bytes") {
            static_cast<net_counter_group *>(pcounters)->tx_bytes = to_integer(value);
        } else if (key == "rx_speed") {
            static_cast<net_counter_group *>(pcounters)->rx_speed = to_double(value);
        } else if (key == "tx_speed") {
            static_cast<net_counter_group *>(pcounters)->tx_speed = to_double(value);
        } else if (key == "rx_speed_max") {
            static_cast<net_counter_group *>(pcounters)->rx_speed_max = to_double(value);
        } else if (key == "tx_speed_max") {
            static_cast<net_counter_group *>(pcounters)->tx_speed_max = to_double(value);
        }

        return false;
    }, & x);

    return result;
}

}} // namespace ionic::metrics