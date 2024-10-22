////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.19 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_net_statistics.hpp"
#include <pfs/ionik/error.hpp>
#include <string>

namespace ionik {
namespace metrics {

class sys_class_net_provider: public basic_net_statistics
{
private:
    std::string _iface;         // network interface name (subdirectory name in /sys/class/net)
    std::string _readable_name;

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
