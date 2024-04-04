////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.04.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/net/network_interface.hpp"
#include "pfs/fmt.hpp"
#include <string>
#include <cerrno>

int main (int , char *[])
{
    auto ifaces = ionik::net::fetch_interfaces([] (ionik::net::network_interface const & iface) -> bool {
        fmt::println("Adapter name: {}\n"
            "\tReadable name: {}\n"
            "\tDescription  : {}\n"
            "\tHW address   : {}\n"
            "\tType         : {}\n"
            "\tStatus       : {}\n"
            "\tMTU          : {}\n"
            "\tIPv4 enabled : {}\n"// << std::boolalpha <<  << "\n";
            "\tIPv6 enabled : {}\n"// << std::boolalpha <<  << "\n";
            "\tIPv4         : {}\n"
            "\tIPv6         : {}\n"
            "\tMulticast    : {}\n\n"
            , iface.adapter_name()
            , iface.readable_name()
            , iface.description()
            , iface.hardware_address()
            , to_string(iface.type())
            , to_string(iface.status())
            , iface.mtu()
            , iface.is_flag_on(ionik::net::network_interface_flag::ip4_enabled)
            , iface.is_flag_on(ionik::net::network_interface_flag::ip6_enabled)
            , iface.ip4_name()
            , iface.ip6_name()
            , iface.is_flag_on(ionik::net::network_interface_flag::multicast));
        return true;
    });

    return EXIT_SUCCESS;
}
