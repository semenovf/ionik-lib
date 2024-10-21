////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/string_view.hpp>
#include <pfs/ionik/error.hpp>
#include <chrono>
#include <cstdint>
#include <vector>

namespace ionik {
namespace metrics {

class basic_net_provider
{
public:
    using string_view = pfs::string_view;
    using time_point_type = std::chrono::time_point<std::chrono::steady_clock>;

    struct counter_group
    {
        std::int64_t rx_bytes {0};    // received bytes totally
        std::int64_t tx_bytes {0};    // transferred bytes totally
        double rx_speed {0};          // receive speed, in bytes per second
        double tx_speed {0};          // transfer speed, in bytes per second
        double rx_speed_max {0};      // max receive speed, in bytes per second
        double tx_speed_max {0};      // max transfer speed, in bytes per second
    };

protected:
    counter_group _recent_data;
    time_point_type _recent_checkpoint; // time point to calculate speed
};

}} // namespace ionik::metrics