////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/counter.hpp"
#include "ionik/metrics/default_counters.hpp"
#include <pfs/bits/operationsystem.h>
#include <pfs/optional.hpp>

#if PFS__OS_WIN
#   include "ionik/metrics/gms_provider.hpp"
#   include "ionik/metrics/netioapi_provider.hpp"
#   include "ionik/metrics/pdh_provider.hpp"
#   include "ionik/metrics/psapi_provider.hpp"
#   include "ionik/metrics/times_provider.hpp"
#else
// #   include "ionik/metrics/proc_meminfo_provider.hpp"
#   include "ionik/metrics/proc_self_status_provider.hpp"
#   include "ionik/metrics/proc_stat_provider.hpp"
#   include "ionik/metrics/sys_class_net_provider.hpp"
#   include "ionik/metrics/sysinfo_provider.hpp"
#   include "ionik/metrics/times_provider.hpp"
#endif

namespace ionik {
namespace metrics {

using string_view = pfs::string_view;

class default_counters::impl
{
private:
    // Provides CPU utilization by the current process
    times_provider _times_provider;

#if PFS__OS_WIN
    // Provides memory usage
    gms_provider _gms_provider;

    // Provides total CPU utilization
    pdh_provider _pdh_provider;

    // Provides memory usage
    psapi_provider _psapi_provider;

    // Network statistics provider
    std::vector<netioapi_provider> _net_providers;

#elif PFS__OS_LINUX
    // Provides total CPU utilization
    proc_stat_provider _stat_provider;

    // Provides memory usage
    sysinfo_provider _sysinfo_provider;

    // Provides self memory usage
    proc_self_status_provider _self_status_provider;

    // Network statistics provider
    std::vector<sys_class_net_provider> _net_providers;
#endif

public:
    impl (error * perr)
        : _times_provider(perr)
#if PFS__OS_WIN
        , _pdh_provider(perr)
#elif PFS__OS_LINUX
        , _stat_provider(perr)
#endif
    {}

public:
    bool query (counter_group & counters, error * perr)
    {
        auto r1 = _times_provider.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
            if (key == "cpu_usage") {
                static_cast<counter_group *>(pcounters)->cpu_usage = to_double(value);
                return true;
            }

            return false;
        }, & counters, perr);

#if PFS__OS_WIN
        auto r2 = _pdh_provider.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
            if (key == "ProcessorTime") {
                static_cast<counter_group *>(pcounters)->cpu_usage_total = to_double(value);
            }

            return false;
            }, & counters, perr);

        auto r3 = _psapi_provider.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
            if (key == "PrivateUsage") {
                static_cast<counter_group *>(pcounters)->mem_usage = to_integer(value);
            }

            return false;
        }, & counters, perr);

        auto r4 = _gms_provider.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
            if (key == "TotalPhys") {
               static_cast<counter_group *>(pcounters)->ram_total = to_integer(value);
            } else if (key == "AvailPhys") {
                static_cast<counter_group *>(pcounters)->ram_free = to_integer(value);
            } else if (key == "TotalSwap") {
                static_cast<counter_group *>(pcounters)->swap_total = to_integer(value);
            } else if (key == "AvailSwap") {
                static_cast<counter_group *>(pcounters)->swap_free = to_integer(value);
            }

            return false;
        }, & counters, perr);
        
        auto success = r1 && r2 && r3 && r4;
#elif PFS__OS_LINUX
        auto r2 = _stat_provider.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
            if (key == "cpu") {
                static_cast<counter_group *>(pcounters)->cpu_usage_total = to_double(value);
                return true;
            }

            return false;
        }, & counters, perr);

        auto r3 = _sysinfo_provider.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
            if (key == "totalram") {
                static_cast<counter_group *>(pcounters)->ram_total = to_integer(value);
            } else if (key == "freeram") {
                static_cast<counter_group *>(pcounters)->ram_free = to_integer(value);
            } else if (key == "totalswap") {
                static_cast<counter_group *>(pcounters)->swap_total = to_integer(value);
            } else if (key == "freeswap") {
                static_cast<counter_group *>(pcounters)->swap_free = to_integer(value);
            }

            return false;
        }, & counters, perr);

        auto r4 = _self_status_provider.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
            if (key == "VmSize") {
                static_cast<counter_group *>(pcounters)->mem_usage = to_integer(value);
            } else if (key == "VmSwap") {
                static_cast<counter_group *>(pcounters)->swap_usage = to_integer(value);
            } else if (key == "VmPeak") {
                static_cast<counter_group *>(pcounters)->mem_peak_usage = to_integer(value);
            }

            return false;
        }, & counters, perr);

        auto success = r1 && r2 && r3 & r4;
#endif

        if (success) {
            if (counters.ram_total && counters.ram_free) {
                counters.ram_usage_total = ((static_cast<double>(*counters.ram_total) - *counters.ram_free)
                    / *counters.ram_total) * 100.0;
            }

            if (counters.swap_total && counters.swap_free) {
                counters.swap_usage_total = ((static_cast<double>(*counters.swap_total) - *counters.swap_free)
                    / *counters.swap_total) * 100.0;
            }
        }

        return success;
    }

    bool query (std::vector<net_counter_group> & counters, error * perr)
    {
        if (_net_providers.empty())
            return true;

        if (counters.size() != _net_providers.size())
            counters.resize(_net_providers.size());

        std::size_t index = 0;
        bool success = true;

        for (auto & x: _net_providers) {
#if PFS__OS_WIN
            netioapi_provider::counter_group tmp;
#elif PFS__OS_LINUX
            sys_class_net_provider::counter_group tmp;
#endif
            if (!x.query(tmp, perr)) {
                success = false;
                break;
            }

            auto ptr = & counters[index++];
            ptr->iface = x.iface_name();
            ptr->readable_name = ptr->iface;
            ptr->rx_bytes = tmp.rx_bytes;
            ptr->rx_bytes = tmp.rx_bytes;
            ptr->rx_speed = tmp.rx_speed;
            ptr->tx_speed = tmp.tx_speed;
            ptr->rx_speed_max = tmp.rx_speed_max;
            ptr->tx_speed_max = tmp.tx_speed_max;
        }

        return success;
    }

    void monitor_net_interface (std::string const & iface, error * perr)
    {
        error err;

#if PFS__OS_WIN
        netioapi_provider provider {iface, & err};
#elif PFS__OS_LINUX
        sys_class_net_provider provider{iface, iface, & err};
#endif

        if (err) {
            pfs::throw_or(perr, std::move(err));
            return;
        }

        _net_providers.push_back(std::move(provider));
    }
};

default_counters::default_counters (error * perr)
    : _d(new impl(perr))
{}

default_counters::default_counters (default_counters &&) noexcept = default;
default_counters & default_counters::operator = (default_counters &&) noexcept = default;
default_counters::~default_counters () = default;

void default_counters::monitor_net_interface (std::string const & iface, error * perr)
{
    _d->monitor_net_interface(iface, perr);
}

void default_counters::monitor_all_net_interfaces (error * perr)
{
    error err;
    auto ifaces = net_interfaces(& err);

    if (!err) {
        for (auto const & iface: ifaces) {
            monitor_net_interface(iface, & err);

            if (err)
                break;
        }
    }

    if (err)
        pfs::throw_or(perr, std::move(err));
}

default_counters::counter_group
default_counters::query (error * perr)
{
    counter_group result;

    if (_d->query(result, perr))
        return result;

    return counter_group{};
}

std::vector<default_counters::net_counter_group>
default_counters::query_net_counters (error * perr)
{
    std::vector<default_counters::net_counter_group> result;

    if (_d->query(result, perr))
        return result;

    return std::vector<default_counters::net_counter_group>{};
}

bool default_counters::query (counter_group & counters, error * perr)
{
    return _d->query(counters, perr);
}

bool default_counters::query (std::vector<net_counter_group> & counters, error * perr)
{
    return _d->query(counters, perr);
}

std::vector<std::string> default_counters::net_interfaces (error * perr)
{
#if PFS__OS_WIN
    return netioapi_provider::interfaces();
#else
    return sys_class_net_provider::interfaces();
#endif
}

}} // namespace ionic::metrics
