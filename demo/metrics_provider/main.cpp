////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#if _MSC_VER
#   include "ionik/metrics/gms_provider.hpp"
#   include "ionik/metrics/pdh_provider.hpp"
#   include "ionik/metrics/psapi_provider.hpp"
#else
#   include "ionik/metrics/proc_provider.hpp"
#   include "ionik/metrics/sysinfo_provider.hpp"
#   include "ionik/metrics/getrusage_provider.hpp"
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

#if _MSC_VER
using ionik::metrics::to_double;
using ionik::metrics::to_integer;

inline bool gms_query (ionik::metrics::gms_provider & gmsp)
{
    return gmsp.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        if (key == "MemoryLoad") {
            LOGD("[gms]", "{}: {} %", key, to_integer(value));
        } else if (key == "TotalPhys") {
            LOGD("[gms]", "{}: {} MB", key, to_integer(value) / (1024 * 1024));
        } else {
            LOGD("[gms]", "{}: {}", key, to_integer(value));
        }
        return false;
    }, nullptr);
}

inline bool pdh_query (ionik::metrics::pdh_provider & pdhp)
{
    return pdhp.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        LOGD("[pdh]", "{}: {}", key,  to_integer(value));
        return false;
    }, nullptr);
}

inline bool psapi_query (ionik::metrics::psapi_provider & psapip)
{
    return psapip.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        if (key == "PrivateUsage" || key == "WorkingSetSize" || key == "PeakWorkingSetSize") {
            LOGD("[psapi]", "{}: {:.1f} MB", key, to_double(value) / (1024 * 1024));
        } else if (key == "PhysicalTotal" || key == "PhysicalAvailable" || key == "SystemCache") {
            LOGD("[psapi]", "{}: {} MB", key, to_integer(value) / (1024 * 1024));
        } else {
            LOGD("[psapi]", "{}: {}", key, to_integer(value));
        }
        return false;
    }, nullptr);
}

#else
inline bool sysinfo_query (ionik::metrics::sysinfo_provider & sip)
{
    return sip.query([] (pfs::string_view key, unsigned long value) {
        if (key == "totalram") {
            LOGD("[sysinfo]", "Total RAM: {:.2f} Gb", static_cast<double>(value) / (1000 * 1000 * 1000));
        } else if (key == "freeram") {
            LOGD("[sysinfo]", "Free RAM: {:.2f} Mb", static_cast<double>(value) / (1024 * 1024));
        }

        return false;
    });
}

inline bool pmp_query (ionik::metrics::proc_meminfo_provider & pmp)
{
    int stop_flag = 3;

    return pmp.query([& stop_flag] (pfs::string_view key, pfs::string_view value, pfs::string_view units) {
        if (key == "MemTotal" || key == "MemFree" || key == "MemAvailable") {
            LOGD("[meminfo]", "{}: {} {}", key, value, units);
            stop_flag--;
        }

        return stop_flag <= 0 ? true : false;
    });
}

inline bool pssp_query (ionik::metrics::proc_self_status_provider & pssp)
{
    int stop_flag = 4;
    return pssp.query([& stop_flag] (pfs::string_view key, std::vector<pfs::string_view> const & values) {
        if (key == "Name" || key == "Pid") {
            LOGD("[self/status]", "{}: {}", key, values[0]);
            stop_flag--;
        } else if (key == "VmSize" || key == "VmRSS") {
            PFS__TERMINATE(values.size() == 2, "");
            LOGD("[self/status]", "{}: {} {}", key, values[0], values[1]);
            stop_flag--;
        }

        return stop_flag <= 0 ? true : false;
    });
}

inline bool rusage_query (ionik::metrics::getrusage_provider & grup)
{
    return grup.query([] (pfs::string_view key, long value) {
        LOGD("[getrusage]", "{}: {}", key, value);
        return false;
    });
}

#endif

int main (int /*argc*/, char * /*argv*/[])
{
    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    std::chrono::seconds query_interval{1};

#if _MSC_VER
    try {
        ionik::metrics::gms_provider gmsp;
        ionik::metrics::pdh_provider pdhp;
        ionik::metrics::psapi_provider psapip;

        while (!TERM_APP && gms_query(gmsp) && pdh_query(pdhp) && psapi_query(psapip)) {
            std::this_thread::sleep_for(query_interval);
        }
    } catch (pfs::error const & ex) {
        LOGE("EXCEPTION", "{}", ex.what());
    }

#else
    ionik::metrics::proc_meminfo_provider pmp;
    ionik::metrics::proc_self_status_provider pssp;
    ionik::metrics::sysinfo_provider sp;
    ionik::metrics::getrusage_provider grup;

    while (!TERM_APP && sysinfo_query(sp) && pmp_query(pmp) && pssp_query(pssp) && rusage_query(grup)) {
        std::this_thread::sleep_for(query_interval);
    }
#endif

    fmt::println("Finishing application");

    return EXIT_SUCCESS;
}
