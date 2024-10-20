////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.19 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/string_view.hpp>
#include <pfs/ionik/error.hpp>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace ionik {
namespace metrics {

class sys_class_net_provider
{
public:
    using string_view = pfs::string_view;
    using time_point_type = std::chrono::time_point<std::chrono::steady_clock>;

    struct counter_group
    {
        std::int64_t rx_bytes {0};     // received bytes totally
        std::int64_t tx_bytes {0};     // transferred bytes totally
        double rx_speed {0};           // receive speed, in bytes per second
        double tx_speed {0};           // transfer speed, in bytes per second
        double rx_speed_max {0};       // max receive speed, in bytes per second
        double tx_speed_max {0};       // max transfer speed, in bytes per second
    };

private:
    std::string _iface;         // network interface name (subdirectory name in /sys/class/net)
    std::string _readable_name;
    counter_group _recent_data;
    time_point_type _recent_checkpoint; // time point to calculate speed

private:
    bool read_all (error * perr);

public:
    sys_class_net_provider (std::string iface, std::string readable_name, error * perr = nullptr);

    ~sys_class_net_provider () = default;
    sys_class_net_provider (sys_class_net_provider &&) = default;
    sys_class_net_provider & operator = (sys_class_net_provider &&) = default;

    sys_class_net_provider (sys_class_net_provider const &) = delete;
    sys_class_net_provider & operator = (sys_class_net_provider const &) = delete;

public:
    std::string const & iface_name () const noexcept
    {
        return _iface;
    }

    std::string const & readable_name () const noexcept
    {
        return _readable_name;
    }

    /**
     * Supported keys:
     *      * rx_bytes - received bytes totally (std::uint64_t);
     *      * tx_bytes - transferred bytes totally (std::uint64_t);
     *      * rx_speed - receive speed, in bytes per second (double);
     *      * tx_speed - transfer speed, in bytes per second (double).
     *      * rx_speed_max - max receive speed, in bytes per second (double);
     *      * tx_speed_max - max transfer speed, in bytes per second (double).
     *
     * @return @c false on failure.
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);

    bool query (counter_group & counters, error * perr = nullptr);

public: // static
    static std::vector<std::string> interfaces (error * perr = nullptr);
};

}} // namespace ionik::metrics
