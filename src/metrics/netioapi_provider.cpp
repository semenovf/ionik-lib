////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/netioapi_provider.hpp"
#include <pfs/i18n.hpp>
#include <pfs/numeric_cast.hpp>
#include <pfs/windows.hpp>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <objbase.h>
#include <cstring>

namespace ionik {
namespace metrics {

netioapi_provider::netioapi_provider (int if_index, error * perr)
    : _if_index(if_index)
{
    if (!read(_recent_data.rx_bytes, _recent_data.tx_bytes, perr))
        return;

    _recent_checkpoint = time_point_type::clock::now();
}

static int index_by_alias (std::string const & alias)
{
    PMIB_IF_TABLE2 iftable = nullptr;

    auto rc = GetIfTable2(& iftable);

    if (rc != NO_ERROR)
        return -1;

    auto walias = pfs::windows::utf8_decode(alias.c_str(), pfs::numeric_cast<int>(alias.size()));

    for (ULONG i = 0; i < iftable->NumEntries; i++) {
        if (walias == iftable->Table[i].Alias)
            return pfs::numeric_cast<int>(iftable->Table[i].InterfaceIndex);
    }

    return -1;
}

netioapi_provider::netioapi_provider (std::string const & alias, error * perr)
{
    auto if_index = index_by_alias(alias);

    if (if_index < 0) {
        pfs::throw_or(perr, error {
              std::make_error_code(std::errc::result_out_of_range)
            , tr::f_("interface not found by alias: {}", alias)
        });

        return;
    }

    _if_index = if_index;

    if (!read(_recent_data.rx_bytes, _recent_data.tx_bytes, perr))
        return;

    _recent_checkpoint = time_point_type::clock::now();
}

bool netioapi_provider::read (std::int64_t & rx_bytes, std::int64_t & tx_bytes, error * perr)
{
    MIB_IF_ROW2 ifrow;

    SecureZeroMemory((PVOID) & ifrow, sizeof(MIB_IF_ROW2));

    ifrow.InterfaceIndex = static_cast<NET_IFINDEX>(_if_index);

    auto rc = GetIfEntry2(& ifrow);

    if (rc != NO_ERROR) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_error
            , tr::_("GetIfEntry2 failure")
            , pfs::system_error_text(rc)
        });

        return false;
    }

    if (_alias.empty()) {
        _alias = pfs::windows::utf8_encode(ifrow.Alias);
        _desc = pfs::windows::utf8_encode(ifrow.Description);
    }

    rx_bytes = pfs::numeric_cast<std::int64_t>(ifrow.InOctets);
    tx_bytes = pfs::numeric_cast<std::int64_t>(ifrow.OutOctets);

    return true;
}

bool netioapi_provider::read_all (error * perr)
{
    std::int64_t rx_bytes = 0;
    std::int64_t tx_bytes = 0;

    if (!read(rx_bytes, tx_bytes, perr))
        return false;

    auto now = time_point_type::clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - _recent_checkpoint).count();

    if (millis <= 0)
        return false;

    auto rx_speed = static_cast<double>(rx_bytes - _recent_data.rx_bytes) * 1000 / millis;
    auto tx_speed = static_cast<double>(tx_bytes - _recent_data.tx_bytes) * 1000 / millis;

    _recent_data.rx_bytes = rx_bytes;
    _recent_data.tx_bytes = tx_bytes;
    _recent_data.rx_speed = rx_speed;
    _recent_data.tx_speed = tx_speed;
    _recent_data.rx_speed_max = (std::max)(_recent_data.rx_speed_max, rx_speed);
    _recent_data.tx_speed_max = (std::max)(_recent_data.tx_speed_max, tx_speed);
    _recent_checkpoint = now;

    return true;
}

std::string const & netioapi_provider::iface_name () const noexcept
{
    return _alias;
}

std::string const & netioapi_provider::readable_name () const noexcept
{
    return _desc;
}

// https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_if_row2
// https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getifentry2

bool netioapi_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    if (f == nullptr)
        return true;

    if (!read_all(perr))
        return false;

    (void)(!f("rx_bytes", counter_t{_recent_data.rx_bytes}, user_data_ptr)
        && !f("tx_bytes", counter_t{_recent_data.tx_bytes}, user_data_ptr)
        && !f("rx_speed", counter_t{_recent_data.rx_speed}, user_data_ptr)
        && !f("tx_speed", counter_t{_recent_data.tx_speed}, user_data_ptr)
        && !f("rx_speed_max", counter_t{_recent_data.rx_speed_max}, user_data_ptr)
        && !f("tx_speed_max", counter_t{_recent_data.tx_speed_max}, user_data_ptr));

    return true;
}

bool netioapi_provider::query (counter_group & counters, error * perr)
{
    if (!read_all(perr))
        return false;

    std::memcpy(& counters, & _recent_data, sizeof(counters));
    return true;
}

std::vector<std::string> netioapi_provider::interfaces (error * perr)
{
    std::vector<std::string> result;

    PMIB_IF_TABLE2 iftable = nullptr;

    auto rc = GetIfTable2(& iftable);

    if (rc != NO_ERROR) {
        pfs::throw_or(perr, error {pfs::errc::unexpected_error, pfs::system_error_text(rc), "GetIfEntry2"});
        return std::vector<std::string>{};
    }

    for (ULONG i = 0; i < iftable->NumEntries; i++) {
        auto accepted = (iftable->Table[i].Type == IF_TYPE_ETHERNET_CSMACD 
            || iftable->Table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK
            || iftable->Table[i].Type == IF_TYPE_IEEE80211 // WiFi
            || iftable->Table[i].Type == IF_TYPE_PPP
            || iftable->Table[i].Type == IF_TYPE_TUNNEL); 

        if (!accepted)
            continue;

        auto alias = pfs::windows::utf8_encode(iftable->Table[i].Alias);
        result.emplace_back(std::move(alias));
    }

    return result;
}

}} // namespace ionic::metrics
