////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.11.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/counter.hpp"
#include "ionik/metrics/network_counters.hpp"
#include <pfs/bits/operationsystem.h>

#if PFS__OS_WIN
#   include "ionik/metrics/netioapi_provider.hpp"
#else
#   include "ionik/metrics/sys_class_net_provider.hpp"
#endif

namespace ionik {
namespace metrics {

class network_counters::impl
{
private:
#if PFS__OS_WIN
    netioapi_provider _net_provider;
#elif PFS__OS_LINUX
    sys_class_net_provider _net_provider;
#endif

public:
    impl (std::string const & iface, error * perr)
#if PFS__OS_WIN
        : _net_provider(iface, perr)
#elif PFS__OS_LINUX
        : _net_provider(iface, iface, perr)
#endif
    {}

public:
    bool query (counter_group & counters, error * perr)
    {
#if PFS__OS_WIN
        netioapi_provider::counter_group tmp;
#elif PFS__OS_LINUX
        sys_class_net_provider::counter_group tmp;
#endif
        if (!_net_provider.query(tmp, perr))
            return false;

        counters.iface = _net_provider.iface_name();
        counters.readable_name = _net_provider.readable_name();
        counters.rx_bytes = tmp.rx_bytes;
        counters.tx_bytes = tmp.tx_bytes;
        counters.rx_speed = tmp.rx_speed;
        counters.tx_speed = tmp.tx_speed;
        counters.rx_speed_max = tmp.rx_speed_max;
        counters.tx_speed_max = tmp.tx_speed_max;

        return true;
    }

public:
    static std::vector<std::string> interfaces (error * perr = nullptr)
    {
#if PFS__OS_WIN
        return netioapi_provider::interfaces(perr);
#elif PFS__OS_LINUX
        return sys_class_net_provider::interfaces(perr);
#endif
    }
};

network_counters::network_counters (error * /*perr*/)
{}

network_counters::network_counters (std::string const & iface, error * perr)
    : _d(new impl(iface, perr))
{}

network_counters::network_counters (network_counters &&) noexcept = default;
network_counters & network_counters::operator = (network_counters &&) noexcept = default;
network_counters::~network_counters () = default;

void network_counters::set_interface (std::string const & iface, error * perr)
{
    _d.reset(new impl(iface, perr));
}

network_counters::counter_group
network_counters::query (error * perr)
{
    if (_d) {
        counter_group result;

        if (_d->query(result, perr))
            return result;
    }

    return counter_group{};
}

bool network_counters::query (counter_group & counters, error * perr)
{
    return _d->query(counters, perr);
}

std::vector<std::string> network_counters::interfaces (error * perr)
{
    return impl::interfaces(perr);
}

}} // namespace ionic::metrics
