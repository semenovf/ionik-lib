////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/times_provider.hpp"
#include "proc_reader.hpp"
#include "parser.hpp"
#include <pfs/bits/operationsystem.h>
#include <pfs/i18n.hpp>

#if PFS__OS_WIN
#   include <pfs/windows.hpp>
#elif PFS__OS_LINUX
#   include <sys/times.h>
#else
#   error "Unsupported OS"
#endif

// References
// 1. [/proc/cpuinfo](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/6/html/deployment_guide/s2-proc-cpuinfo)

namespace ionik {
namespace metrics {

using string_view = pfs::string_view;

#if PFS__OS_LINUX

struct record_view
{
    string_view key;
    string_view value;
};

static bool parse_record (string_view::const_iterator & pos, string_view::const_iterator last
    , record_view & rec, error * perr)
{
    auto p = pos;

    if (p == last)
        return false;

    skip_ws(p, last);

    if (p == last)
        return false;

    rec.key = string_view{};
    rec.value = string_view{};

    // Empty string, ignore
    if (is_nl(*p)) {
        pos = p;
        advance_nl1n(pos, last);
        return true;
    }

    auto success = advance_key(p, last, rec.key)
        && advance_ws0n(p, last)
        && advance_colon(p, last)
        && advance_ws0n(p, last)
        && advance_unparsed_value(p, last, rec.value)
        && advance_nl1n(p, last);

    if (!success) {
        pfs::throw_or(perr, tr::_("'/proc/cpuinfo' record has unexpected format"));
        return false;
    }

    // Value can be empty, but not key
    if (rec.key.empty()) {
        pfs::throw_or(perr, tr::_("'/proc/cpuinfo' record key is empty"));
        return false;
    }

    pos = p;
    return true;
}
#endif

times_provider::times_provider (error * perr)
{
#if PFS__OS_WIN
    SYSTEM_INFO sys_info;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(& sys_info);
    _core_count = static_cast<std::uint32_t>(sys_info.dwNumberOfProcessors);

    GetSystemTimeAsFileTime(& ftime);
    memcpy(& _recent_time, & ftime, sizeof(FILETIME));

    GetProcessTimes(GetCurrentProcess(), & ftime, & ftime, & fsys, & fuser);
    memcpy(& _recent_sys, & fsys, sizeof(FILETIME));
    memcpy(& _recent_usr, & fuser, sizeof(FILETIME));
#elif PFS__OS_LINUX
    using pfs::to_string;

    error err;
    proc_reader reader {PFS__LITERAL_PATH("/proc/cpuinfo"), & err};

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return;
    }

    auto content = reader.move_content();

    int cpu_index = -1;
    _cpu_info.reserve(4);
    cpu_core_info * pcore = nullptr;

    string_view content_view {content};
    auto pos = content_view.cbegin();
    auto last = content_view.cend();

    record_view rec;

    while (parse_record(pos, last, rec, & err)) {
        if (err) {
            pfs::throw_or(perr, std::move(err));
            return;
        }

        if (rec.key.empty()) {
            pcore = nullptr;
            continue;
        }

        if (rec.key == "processor") {
            cpu_index++;
            _cpu_info.emplace_back(cpu_core_info{});
            pcore = & _cpu_info[cpu_index];
            continue;
        }

        if (pcore == nullptr) {
            pfs::throw_or(perr, tr::_("`/proc/cpuinfo` has unexpected format"));
            return;
        }

        if (rec.key == "vendor_id") {
            pcore->vendor_id = to_string(rec.value);
        } else if (rec.key == "model_name") {
            pcore->model_name = to_string(rec.value);
        } else if (rec.key == "cache_size") {
            string_view value;
            string_view units;
            auto pos = rec.value.cbegin();
            auto last = rec.value.cend();
            auto success = advance_decimal_digits_value(pos, last, value)
                && advance_units(pos, last, units);

            if (success) {
                pcore->cache_size = to_int64_counter(value, units, & err);

                if (!err)
                    success = false;
            }

            if (!success) {
                pfs::throw_or(perr, tr::_("`cache_size` in `/proc/cpuinfo` has unexpected format"));
                return;
            }
        }
    }

    struct tms ticks;
    _recent_ticks = times(& ticks);

    if (_recent_ticks == clock_t{-1}) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , tr::f_("times() call failure")
        });

        return;
    }

    _recent_sys = ticks.tms_stime;
    _recent_usr = ticks.tms_utime;
#endif
}

pfs::optional<double> times_provider::calculate_cpu_usage ()
{
    double result {-1};

#if PFS__OS_WIN
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;

    GetSystemTimeAsFileTime(& ftime);
    memcpy(& now, & ftime, sizeof(FILETIME));

    GetProcessTimes(GetCurrentProcess(), & ftime, & ftime, & fsys, & fuser);
    memcpy(& sys, & fsys, sizeof(FILETIME));
    memcpy(& user, & fuser, sizeof(FILETIME));

    result = static_cast<double>((sys.QuadPart - _recent_sys.QuadPart) + (user.QuadPart - _recent_usr.QuadPart));
    result /= (now.QuadPart - _recent_time.QuadPart);
    result /= _core_count;
    _recent_time = now;
    _recent_usr  = user;
    _recent_sys  = sys;

    result *= 100;
#elif PFS__OS_LINUX
    auto const cpu_count = _cpu_info.size();
    struct tms ticks;

    PFS__TERMINATE(cpu_count > 0, "");

    clock_t now = times(& ticks);

    if (now == clock_t{-1})
        return pfs::nullopt;

    auto overflow = now <= _recent_ticks
        || ticks.tms_stime < _recent_sys
        || ticks.tms_utime < _recent_usr;

    if (overflow) {
        result = double{-1};
    } else {
        //Overflow detection. Just skip this value.
        result = (ticks.tms_stime - _recent_sys) + (ticks.tms_utime - _recent_usr);
        result /= (now - _recent_ticks);
        result /= cpu_count;
        result *= 100;
    }

    _recent_ticks = now;
    _recent_sys = ticks.tms_stime;
    _recent_usr = ticks.tms_utime;
#endif

    return result;
}

bool times_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    error err;
    auto opt_cpu_usage = calculate_cpu_usage();

    if (!opt_cpu_usage) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , tr::_("times() call failure")
        });

        return false;
    }

    // Skip
    if (*opt_cpu_usage < double{0})
        return true;

    if (f != nullptr)
        f("cpu_usage", counter_t{*opt_cpu_usage}, user_data_ptr);

    return true;
}

}} // namespace ionic::metrics

