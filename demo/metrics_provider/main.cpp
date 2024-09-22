////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#ifndef _MSC_VER
#   include "ionik/metrics/proc_provider.hpp"
#   include "ionik/metrics/sysinfo_provider.hpp"
#endif

#include <pfs/assert.hpp>
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

#ifndef _MSC_VER
inline bool pmp_query (ionik::metrics::proc_meminfo_provider & pmp)
{
    return pmp.query([] (pfs::string_view const & key, pfs::string_view const & value, pfs::string_view const & units) {
        LOGD("[meminfo]", "{}: {} {}", key, value, units);
        return false;
    });
}

inline bool pssp_query (ionik::metrics::proc_self_status_provider & pssp)
{
    int stop_flag = 0;
    return pssp.query([& stop_flag] (pfs::string_view const & key, std::vector<pfs::string_view> const & values) {
        if (key == "Name" || key == "Pid") {
            LOGD("[self/status]", "{}: {}", key, values[0]);
            stop_flag++;
        } else if (key == "VmSize" || key == "VmRSS") {
            PFS__TERMINATE(values.size() == 2, "");
            LOGD("[self/status]", "{}: {} {}", key, values[0], values[1]);
            stop_flag++;
        }

        return stop_flag >= 4 ? true : false;
    });
}

#endif

int main (int /*argc*/, char * /*argv*/[])
{
    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    std::chrono::seconds query_interval{1};

#ifndef _MSC_VER
    ionik::metrics::proc_meminfo_provider pmp;
    ionik::metrics::proc_self_status_provider pssp;
    ionik::metrics::sysinfo_provider sp;

    while (!TERM_APP && sp.query() && pmp_query(pmp) && pssp_query(pssp)) {
        std::this_thread::sleep_for(query_interval);
    }
#endif

    fmt::println("Finishing application");

    return EXIT_SUCCESS;
}
