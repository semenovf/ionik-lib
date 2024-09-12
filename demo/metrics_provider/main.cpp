////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/proc_provider.hpp"
#include "ionik/metrics/sysinfo_provider.hpp"
#include <pfs/i18n.hpp>
#include <pfs/log.hpp>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <signal.h>

static std::atomic_bool TERM_APP {false};

static void sigterm_handler (int /*sig*/)
{
    TERM_APP = true;
}

inline bool pmp_query (ionik::metrics::proc_meminfo_provider & pmp)
{
    return pmp.query([] (std::string const & key, std::string const & value, std::string const & units) {
        LOGD("[meminfo]", "{}: {} {}", key, value, units);
    });
}

int main (int /*argc*/, char * /*argv*/[])
{
    std::chrono::seconds query_interval{1};
    ionik::metrics::proc_meminfo_provider pmp;
    ionik::metrics::sysinfo_provider sp;

    while (!TERM_APP && sp.query() && pmp_query(pmp)) {
        std::this_thread::sleep_for(query_interval);
    }

    fmt::println("Finishing application");

    return EXIT_SUCCESS;
}
