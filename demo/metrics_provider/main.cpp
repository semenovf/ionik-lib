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
#   include "ionik/metrics/proc_meminfo_provider.hpp"
#   include "ionik/metrics/proc_self_status_provider.hpp"
#   include "ionik/metrics/proc_stat_provider.hpp"
#   include "ionik/metrics/sysinfo_provider.hpp"
#   include "ionik/metrics/times_provider.hpp"
#   include "ionik/metrics/getrusage_provider.hpp"
#endif

#include "ionik/metrics/default_counters.hpp"
#include "ionik/metrics/random_counters.hpp"
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

using ionik::metrics::to_double;
using ionik::metrics::to_integer;

constexpr double to_kibs (std::int64_t value)
{
    return static_cast<double>(value) / 1024;
}

constexpr double to_mibs (std::int64_t value)
{
    return static_cast<double>(value) / (double{1024} * 1024);
}

#if _MSC_VER
inline bool gms_query (ionik::metrics::gms_provider & gmsp)
{
    return gmsp.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        if (key == "MemoryLoad") {
            LOGD("[gms]", "{}: {} %", key, to_integer(value));
        } else if (key == "TotalPhys") {
            LOGD("[gms]", "{}: {} MiB", key, to_mibs(to_integer(value)));
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
            LOGD("[psapi]", "{}: {:.2f} MiB", key, to_mibs(to_integer(value)));
        } else if (key == "PhysicalTotal" || key == "PhysicalAvailable" || key == "SystemCache") {
            LOGD("[psapi]", "{}: {:.2f} MiB", key, to_mibs(to_integer(value)));
        } else {
            LOGD("[psapi]", "{}: {:.2f}", key, to_integer(value));
        }
        return false;
    }, nullptr);
}

#else
inline bool sysinfo_query (ionik::metrics::sysinfo_provider & sip)
{
    return sip.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        if (key == "totalram") {
            LOGD("[sysinfo]", "Total RAM: {:.2f} Gb", to_double(value) / (1000 * 1000 * 1000));
        } else if (key == "freeram") {
            LOGD("[sysinfo]", "Free RAM: {:.2f} Mb", to_double(value) / (1000 * 1000));
        }

        return false;
    }, nullptr);
}

inline bool pmp_query (ionik::metrics::proc_meminfo_provider & pmp)
{
    return pmp.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        LOGD("[meminfo]", "{}: {} Kb", key, to_integer(value) / 1000);
        return false;
    }, nullptr);
}

inline bool pssp_query (ionik::metrics::proc_self_status_provider & pssp)
{
    return pssp.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        LOGD("[self/status]", "{}: {} Kb", key, to_integer(value) / 1000);
        return false;
    }, nullptr);
}

inline bool psp_query (ionik::metrics::proc_stat_provider & psp)
{
    return psp.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        LOGD("[stat]", "{}: {:.2f} %", key, to_double(value));
        return false;
    }, nullptr);
}

inline bool tp_query (ionik::metrics::times_provider & ptp)
{
    return ptp.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        if (key == "cpu_usage") {
            LOGD("[times]", "{}: {} %", key, to_integer(value));
        } else {
            LOGD("[times]", "{}: {}", key, to_integer(value));
        }
        return false;
    }, nullptr);
}

inline bool rusage_query (ionik::metrics::getrusage_provider & grup)
{
    return grup.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void *) -> bool {
        LOGD("[getrusage]", "{}: {}", key, to_integer(value));
        return false;
    }, nullptr);
}

#endif

template <typename CountersType>
bool default_query (int counter, CountersType & dc)
{
    ionik::error err;
    auto counters = dc.query(& err);

    if (err) {
        LOGE("", "{}", err.what());
        return false;
    }

    LOGD("[default]", "-- Iteration: {:>4} {:-<{}}", counter, "", 60);

    if (counters.cpu_usage_total)
        LOGD("[default]", "{:<22}: {:.2f} %", "CPU usage total", *counters.cpu_usage_total);

    if (counters.cpu_usage)
        LOGD("[default]", "{:<22}: {:.2f} %", "Process CPU usage", *counters.cpu_usage);

    if (counters.ram_total)
        LOGD("[default]", "{:<22}: {:.2f} MiB", "RAM total", to_mibs(*counters.ram_total));

    if (counters.ram_free)
        LOGD("[default]", "{:<22}: {:.2f} MiB", "RAM free", to_mibs(*counters.ram_free));

    if (counters.ram_usage_total)
        LOGD("[default]", "{:<22}: {:.2f} %", "RAM usage total", *counters.ram_usage_total);

    if (counters.swap_total)
        LOGD("[default]", "{:<22}: {:.2f} MiB", "Swap total", to_mibs(*counters.swap_total));

    if (counters.swap_free)
        LOGD("[default]", "{:<22}: {:.2f} MiB", "Swap free", to_mibs(*counters.swap_free));

    if (counters.swap_usage_total)
        LOGD("[default]", "{:<22}: {:.2f} %", "Swap usage total", *counters.swap_usage_total);

    if (counters.mem_usage)
        LOGD("[default]", "{:<22}: {:.2f} KiB", "Process memory usage", to_kibs(*counters.mem_usage));

    if (counters.mem_peak_usage)
        LOGD("[default]", "{:<22}: {:.2f} KiB", "Peak memory usage", to_kibs(*counters.mem_peak_usage));

    if (counters.swap_usage)
        LOGD("[default]", "{:<22}: {:.2f} KiB", "Process swap usage", to_kibs(*counters.swap_usage));

    return true;
}

int main (int argc, char * argv[])
{
    if (argc > 1 && pfs::string_view{argv[1]} == "--help") {
        fmt::println("{} [--help | --verbose | --random]", argv[0]);
        return EXIT_SUCCESS;
    }

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    std::chrono::seconds query_interval{1};

    std::thread busy_thread {[] {
        std::int64_t counter = 0;

        while (!TERM_APP) {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    }};

    if (argc > 1 && pfs::string_view{argv[1]} == "--verbose") {
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
        ionik::metrics::proc_stat_provider psp;
        ionik::metrics::times_provider tp;
        ionik::metrics::sysinfo_provider sp;
        ionik::metrics::getrusage_provider grup;

        while (!TERM_APP && sysinfo_query(sp) && pmp_query(pmp) && pssp_query(pssp)
            && psp_query(psp) && tp_query(tp) && rusage_query(grup)) {

            std::this_thread::sleep_for(query_interval);
        }
#endif
    } else if (argc > 1 && pfs::string_view{argv[1]} == "--random") {
        int counter = 0;
        ionik::metrics::random_counters rc;

        while (!TERM_APP && default_query(++counter, rc)) {
            std::this_thread::sleep_for(query_interval);
        }
    } else {
        int counter = 0;
        ionik::metrics::default_counters dc;

        while (!TERM_APP && default_query(++counter, dc)) {
            std::this_thread::sleep_for(query_interval);
        }
    }

    busy_thread.join();

    LOGD("", "Finishing application");

    return EXIT_SUCCESS;
}
