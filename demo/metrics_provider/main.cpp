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
#   include "ionik/metrics/netioapi_provider.hpp"
#   include "ionik/metrics/pdh_provider.hpp"
#   include "ionik/metrics/psapi_provider.hpp"
#else
#   include "ionik/metrics/proc_meminfo_provider.hpp"
#   include "ionik/metrics/proc_self_status_provider.hpp"
#   include "ionik/metrics/proc_stat_provider.hpp"
#   include "ionik/metrics/sysinfo_provider.hpp"
#   include "ionik/metrics/times_provider.hpp"
#   include "ionik/metrics/getrusage_provider.hpp"
#   include "ionik/metrics/sys_class_net_provider.hpp"
#endif

#include "ionik/metrics/default_counters.hpp"
#include "ionik/metrics/random_counters.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <pfs/log.hpp>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <vector>
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

template <typename NetProvider>
inline bool net_query (std::vector<NetProvider> & net)
{
    bool success = true;

    for (auto & x: net) {
        auto iface = x.iface_name();
        x.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void * user_data_ptr) -> bool {
            auto iface_ptr = static_cast<std::string const *>(user_data_ptr);
            std::string tag = '[' + *iface_ptr + ']';
            LOGD(tag, "{}: {:.2f}", key, to_double(value));
            return false;
        }, & iface);
    }

    return success;
}

template <typename CountersType>
bool default_query (int counter, CountersType & dc)
{
    ionik::error err;

    auto counters = dc.query(& err);

    if (err) {
        LOGE("", "{}", err.what());
        return false;
    }

    auto net_counters = dc.query_net_counters(& err);

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

    for (auto const & x: net_counters) {
        std::string tag = '[' + x.iface + ']';

        LOGD(tag, "{:<22}: {:.2f} KiB", "Received", to_kibs(x.rx_bytes));
        LOGD(tag, "{:<22}: {:.2f} KiB", "Transferred", to_kibs(x.tx_bytes));
        LOGD(tag, "{:<22}: {:.2f} bps", "Receive speed", x.rx_speed);
        LOGD(tag, "{:<22}: {:.2f} bps", "Transfer speed", x.tx_speed);
        LOGD(tag, "{:<22}: {:.2f} bps", "Max receive speed", x.rx_speed_max);
        LOGD(tag, "{:<22}: {:.2f} bps", "Max transfer speed", x.tx_speed_max);
    }

    return true;
}

int main (int argc, char * argv[])
{
    if (argc > 1 && pfs::string_view{argv[1]} == "--help") {
        fmt::println("{} [--help | --verbose | --random | --net-interfaces]", argv[0]);
        return EXIT_SUCCESS;
    }

    if (argc > 1 && pfs::string_view{argv[1]} == "--net-interfaces") {
        auto ifaces = ionik::metrics::default_counters::net_interfaces();

        fmt::println("Network interfaces available:");

        for (auto const & iface: ifaces)
            fmt::println("  {}", iface);

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
            std::vector<ionik::metrics::netioapi_provider> net;

            for (auto const & iface: ionik::metrics::netioapi_provider::interfaces()) {
                net.emplace_back(iface);
            }

            while (!TERM_APP && gms_query(gmsp) && pdh_query(pdhp) && psapi_query(psapip) 
                && net_query<ionik::metrics::netioapi_provider>(net)) {

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
        std::vector<ionik::metrics::sys_class_net_provider> net;

        for (auto const & iface: ionik::metrics::sys_class_net_provider::interfaces()) {
            net.emplace_back(iface, iface);
        }

        while (!TERM_APP && sysinfo_query(sp) && pmp_query(pmp) && pssp_query(pssp)
            && psp_query(psp) && tp_query(tp) && rusage_query(grup) 
            && net_query<ionik::metrics::sys_class_net_provider>(net)) {

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
        dc.monitor_all_net_interfaces();

        while (!TERM_APP && default_query(++counter, dc)) {
            std::this_thread::sleep_for(query_interval);
        }
    }

    busy_thread.join();

    LOGD("", "Finishing application");

    return EXIT_SUCCESS;
}
