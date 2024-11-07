////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.11.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/ionik/error.hpp>
#include <pfs/ionik/exports.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ionik {
namespace metrics {

class network_counters
{
    class impl;

public:
    struct counter_group
    {
        std::string iface;         // network interface name (subdirectory name in /sys/class/net for Linux and LUID (locally unique identifier) for Windows)
        std::string readable_name; // network interface readable name
        std::int64_t rx_bytes;     // Received bytes totally
        std::int64_t tx_bytes;     // Transferred bytes totally
        double rx_speed;           // Receive speed, in bytes per second
        double tx_speed;           // Transfer speed, in bytes per second
        double rx_speed_max;       // Max receive speed, in bytes per second
        double tx_speed_max;       // Max transfer speed, in bytes per second
    };

private:
    std::unique_ptr<impl> _d;

public:
    IONIK__EXPORT network_counters (std::string const & iface, error * perr = nullptr);
    IONIK__EXPORT ~network_counters ();
    IONIK__EXPORT network_counters (network_counters &&) noexcept;
    IONIK__EXPORT network_counters & operator = (network_counters &&) noexcept;

    network_counters (network_counters const &) = delete;
    network_counters & operator = (network_counters const &) = delete;

public:
    IONIK__EXPORT counter_group query (error * perr = nullptr);
    IONIK__EXPORT bool query (counter_group & counters, error * perr = nullptr);

public: // static
    IONIK__EXPORT static std::vector<std::string> interfaces (error * perr = nullptr);
};

}} // namespace ionik::metrics

