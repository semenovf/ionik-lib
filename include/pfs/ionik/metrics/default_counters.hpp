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
#include <pfs/ionik/exports.hpp>
#include <pfs/optional.hpp>
#include <memory>
#include <string>
#include <vector>

namespace ionik {
namespace metrics {

class default_counters
{
    class impl;

public:
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
        pfs::optional<std::int64_t> mem_peak_usage; // Peak memory usage by the current process, in bytes (Linux only)
        pfs::optional<std::int64_t> swap_usage;     // Current process swap usage, in bytes
    };

    struct net_counter_group
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
    IONIK__EXPORT default_counters (error * perr = nullptr);
    IONIK__EXPORT ~default_counters ();
    IONIK__EXPORT default_counters (default_counters &&) noexcept;
    IONIK__EXPORT default_counters & operator = (default_counters &&) noexcept;

    default_counters (default_counters const &) = delete;
    default_counters & operator = (default_counters const &) = delete;

public:
    /**
     * Add specified network interface @a iface to monitoring statistics. List of available
     * interfaces can be obtained by default_counters::net_interfaces() method call.
     *
     * @see default_counters::net_interfaces.
     */
    IONIK__EXPORT void monitor_net_interface (std::string const & iface, error * perr = nullptr);

    IONIK__EXPORT void monitor_all_net_interfaces (error * perr = nullptr);

    IONIK__EXPORT counter_group query (error * perr = nullptr);
    IONIK__EXPORT std::vector<net_counter_group> query_net_counters (error * = nullptr);

    IONIK__EXPORT bool query (counter_group & counters, error * perr = nullptr);
    IONIK__EXPORT bool query (std::vector<net_counter_group> & counters, error * perr = nullptr);

public: // static
    IONIK__EXPORT static std::vector<std::string> net_interfaces (error * perr = nullptr);
};

}} // namespace ionik::metrics
