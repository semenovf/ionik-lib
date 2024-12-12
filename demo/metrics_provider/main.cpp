////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#if _MSC_VER
#   include "pfs/ionik/metrics/gms_provider.hpp"
#   include "pfs/ionik/metrics/netioapi_provider.hpp"
#   include "pfs/ionik/metrics/pdh_provider.hpp"
#   include "pfs/ionik/metrics/psapi_provider.hpp"
#elif __linux__
#   include "pfs/ionik/metrics/freedesktop_provider.hpp"
#   include "pfs/ionik/metrics/proc_meminfo_provider.hpp"
#   include "pfs/ionik/metrics/proc_self_status_provider.hpp"
#   include "pfs/ionik/metrics/proc_stat_provider.hpp"
#   include "pfs/ionik/metrics/sysinfo_provider.hpp"
#   include "pfs/ionik/metrics/times_provider.hpp"
#   include "pfs/ionik/metrics/getrusage_provider.hpp"
#   include "pfs/ionik/metrics/sys_class_net_provider.hpp"
#else
#   error "Unsupported operation system"
#endif

#include "pfs/ionik/metrics/system_counters.hpp"
#include "pfs/ionik/metrics/network_counters.hpp"
#include "pfs/ionik/metrics/random_counters.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <pfs/log.hpp>
#include <pfs/memory.hpp>
#include <atomic>
#include <cstdlib>
#include <memory>
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

static void print_help (char const * program)
{
    fmt::println("{} --help", program);
    fmt::println("{} --net-interfaces", program);
    fmt::println("{} [--verbose | --random] [--iface IFACE]", program);
    fmt::println("{} --os", program);
}

static void print_net_interfaces ()
{
    int counter = 1;
    auto ifaces = ionik::metrics::network_counters::interfaces();

    fmt::println("Network interfaces available:");

    for (auto const & iface: ifaces)
        fmt::println("  {}. {}", counter++, iface);
}

static void print_os ()
{
#if __linux__
    ionik::metrics::freedesktop_provider fp;
    auto const & osr = fp.os_release();
    fmt::println("OS           : {}", osr.name);
    fmt::println("OS name      : {}", osr.pretty_name);
    fmt::println("OS version   : {}", osr.version);
    fmt::println("OS version ID: {}", osr.version_id);
    fmt::println("OS codename  : {}", osr.codename);
    fmt::println("OS ID        : {}", osr.id);
    fmt::println("OS ID LIKE   : {}", osr.id_like);
#endif
}

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
inline bool net_provider_query (NetProvider & net)
{
    bool success = true;

    auto iface = net.iface_name();
    net.query([] (pfs::string_view key, ionik::metrics::counter_t const & value, void * user_data_ptr) -> bool {
        auto iface_ptr = static_cast<std::string const *>(user_data_ptr);
        std::string tag = '[' + *iface_ptr + ']';
        LOGD(tag, "{}: {:.2f}", key, to_double(value));
        return false;
    }, & iface);

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

template <typename CountersType>
bool network_query (CountersType & net)
{
    ionik::error err;

    auto counters = net.query(& err);

    if (err) {
        LOGE("", "{}", err.what());
        return false;
    }

    std::string tag = '[' + counters.iface + ']';

    LOGD(tag, "{:<22}: {}"        , "Name", counters.iface);
    LOGD(tag, "{:<22}: {:.2f} KiB", "Received", to_kibs(counters.rx_bytes));
    LOGD(tag, "{:<22}: {:.2f} KiB", "Transferred", to_kibs(counters.tx_bytes));
    LOGD(tag, "{:<22}: {:.2f} bps", "Receive speed", counters.rx_speed);
    LOGD(tag, "{:<22}: {:.2f} bps", "Transfer speed", counters.tx_speed);
    LOGD(tag, "{:<22}: {:.2f} bps", "Max receive speed", counters.rx_speed_max);
    LOGD(tag, "{:<22}: {:.2f} bps", "Max transfer speed", counters.tx_speed_max);

    return true;
}


int main (int argc, char * argv[])
{
    bool is_random_counters = false;
    bool is_verbose = false;
    std::string iface;

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return EXIT_SUCCESS;
        } else if (std::strcmp(argv[i], "--net-interfaces") == 0) {
            print_net_interfaces();
            return EXIT_SUCCESS;
        } else if (std::strcmp(argv[i], "--os") == 0) {
            print_os();
            return EXIT_SUCCESS;
        } else if (std::strcmp(argv[i], "--random") == 0) {
            is_random_counters = true;
        } else if (std::strcmp(argv[i], "--verbose") == 0) {
            is_verbose = true;
        } else if (std::strcmp(argv[i], "--iface") == 0) {
            if (i + 1 < argc) {
                iface = argv[i + 1];
                i++;
            } else {
                fmt::println(stderr, "Error: expected network interface for --iface option");
                return EXIT_FAILURE;
            }
        }
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

    if (is_verbose) {
        try {
#if _MSC_VER
            ionik::metrics::gms_provider gmsp;
            ionik::metrics::pdh_provider pdhp;
            ionik::metrics::psapi_provider psapip;
            auto net = iface.empty()
                ? std::unique_ptr<ionik::metrics::netioapi_provider>{}
                : pfs::make_unique<ionik::metrics::netioapi_provider>(iface);

            while (!TERM_APP && gms_query(gmsp) && pdh_query(pdhp) && psapi_query(psapip)) {
                if (net) {
                    if (!net_provider_query<ionik::metrics::netioapi_provider>(*net))
                        break;
                }

                std::this_thread::sleep_for(query_interval);
            }
#else
            ionik::metrics::proc_meminfo_provider pmp;
            ionik::metrics::proc_self_status_provider pssp;
            ionik::metrics::proc_stat_provider psp;
            ionik::metrics::times_provider tp;
            ionik::metrics::sysinfo_provider sp;
            ionik::metrics::getrusage_provider grup;
            auto net = iface.empty()
                ? std::unique_ptr<ionik::metrics::sys_class_net_provider>{}
                : pfs::make_unique<ionik::metrics::sys_class_net_provider>(iface, iface);

            while (!TERM_APP && sysinfo_query(sp) && pmp_query(pmp) && pssp_query(pssp)
                && psp_query(psp) && tp_query(tp) && rusage_query(grup)) {

                if (net) {
                    if (!net_provider_query<ionik::metrics::sys_class_net_provider>(*net))
                        break;
                }

                std::this_thread::sleep_for(query_interval);
            }
#endif
        } catch (pfs::error const & ex) {
            LOGE("EXCEPTION", "{}", ex.what());
            TERM_APP = true;
        }
    } else if (is_random_counters) {
        int counter = 0;
        ionik::metrics::random_system_counters rsc;
        ionik::metrics::random_network_counters rnc;

        while (!TERM_APP && default_query(++counter, rsc) && network_query(rnc)) {
            std::this_thread::sleep_for(query_interval);
        }
    } else {
        try {
            int counter = 0;
            ionik::metrics::system_counters dc;
            ionik::metrics::network_counters nc;

            if (!iface.empty())
                nc.set_interface(iface);

            while (!TERM_APP && default_query(++counter, dc) && network_query(nc)) {
                std::this_thread::sleep_for(query_interval);
            }
        } catch (pfs::error const & ex) {
            LOGE("EXCEPTION", "{}", ex.what());
            TERM_APP = true;
        }
    }

    busy_thread.join();

    LOGD("", "Finishing application");

    return EXIT_SUCCESS;
}
