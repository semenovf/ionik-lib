////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.02.15 Initial version (netty-lib).
//      2024.04.04 Moved from `netty-lib`.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/net/netlink_socket.hpp"
#include "pfs/ionik/net/netlink_monitor.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/log.hpp"
#include "pfs/string_view.hpp"

using string_view = pfs::string_view;
static char const * TAG = "netty";

namespace fs = pfs::filesystem;
using netlink_attributes = ionik::net::netlink_attributes;
using netlink_socket     = ionik::net::netlink_socket;
using netlink_monitor    = ionik::net::netlink_monitor;

static struct program_context {
    std::string program;
} __pctx;

static void print_usage ()
{
    fmt::print(stdout, "Usage\n{}\n", __pctx.program);
//     fmt::print(stdout, "\t--discovery-saddr=ADDR:PORT\n");
//     fmt::print(stdout, "\t--target-saddr=ADDR:PORT...\n");
}

int main (int argc, char * argv[])
{
    __pctx.program = fs::utf8_encode(fs::utf8_decode(argv[0]).filename());

    //if (argc == 1) {
        // print_usage();
        // return EXIT_SUCCESS;
    //}

    for (int i = 1; i < argc; i++) {
        if (string_view{"-h"} == argv[i] || string_view{"--help"} == argv[i]) {
            print_usage();
            return EXIT_SUCCESS;
        } else {
            ;
        }
    }

    fmt::print("Start Netlink monitoring\n");

    netlink_monitor nm;

    nm.attrs_ready = [] (netlink_attributes const & attrs) {
        fmt::print("{} [{}] [{}]: mtu={}\n", attrs.iface_name
            , attrs.running ? "RUNNING" : "NOT RUNNING"
            , attrs.up ? "UP" : "DOWN"
            , attrs.mtu);
    };

    nm.inet4_addr_added = [] (std::uint32_t addr, std::uint32_t iface_index) {
        LOGD("", "Address added to interface {}: {}", iface_index, addr);
    };

    nm.inet4_addr_removed = [] (std::uint32_t addr, std::uint32_t iface_index) {
        LOGD("", "Address removed from interface {}: {}", iface_index, addr);
    };

    while(true)
        nm.poll(std::chrono::seconds{1});

    return EXIT_SUCCESS;
}
