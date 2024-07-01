////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.06.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/filesystem_monitor/platform_monitor.hpp"
#include "pfs/ionik/filesystem_monitor/callbacks/functional.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/fmt.hpp>
#include <atomic>
#include <signal.h>

using filesystem_monitor = ionik::filesystem_monitor::monitor_t;

static std::atomic_bool TERM_APP {false};

static void sigterm_handler (int /*sig*/)
{
    TERM_APP = true;
}

int main (int argc, char * argv[])
{
    if (argc < 2) {
        fmt::println(stderr, "ERROR: Too few arguments");
        return EXIT_FAILURE;
    }

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    auto path = pfs::filesystem::utf8_decode(argv[1]);
    ionik::filesystem_monitor::functional_callbacks callbacks;
    filesystem_monitor mon;

    fmt::println("Watching: {}", path);
    mon.add(path);

    std::chrono::seconds timeout{30};

    while (!TERM_APP && mon.poll(timeout, callbacks) >= 0) {
        ;
    }

    fmt::println("Finishing application");

    return EXIT_SUCCESS;
}
