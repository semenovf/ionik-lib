////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/process_times_provider.hpp"
#include "ionik/metrics/proc_provider.hpp"
#include "parser.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <pfs/split.hpp>
#include <sys/times.h>

// References
// 1. [/proc/cpuinfo](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/6/html/deployment_guide/s2-proc-cpuinfo)

namespace ionik {
namespace metrics {

using string_view = pfs::string_view;

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
        advance_nl(pos, last);
        return true;
    }

    auto success = advance_key(p, last, rec.key)
        && advance_ws(p, last)
        && advance_colon(p, last)
        && advance_ws(p, last)
        && advance_unparsed_value(p, last, rec.value)
        && advance_nl(p, last);

    if (!success) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_data
            , tr::_("`/proc/cpuinfo` record has unexpected format")
        });

        return false;
    }

    // Value can be empty, but not key
    if (rec.key.empty()) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_data
            , tr::_("`/proc/cpuinfo` record key is empty")
        });

        return false;
    }

    pos = p;
    return true;
}

process_times_provider::process_times_provider (error * perr)
{
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
            pfs::throw_or(perr, error {
                  pfs::errc::unexpected_data
                , tr::_("`/proc/cpuinfo` has unexpected format")
            });

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
                pfs::throw_or(perr, error {
                      pfs::errc::unexpected_data
                    , tr::_("`cache_size` in `/proc/cpuinfo` has unexpected format")
                });

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
}

double process_times_provider::calculate_cpu_usage (error * perr)
{
    auto const cpu_count = _cpu_info.size();
    struct tms ticks;
    double x = -1.0;

    PFS__TERMINATE(cpu_count > 0, "");

    clock_t now = times(& ticks);

    if (now == clock_t{-1}) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , tr::f_("times() call failure")
        });

        return double{-1.0};
    }

    auto success = now > _recent_ticks
        && ticks.tms_stime >= _recent_sys
        && ticks.tms_utime >= _recent_usr;

    if (success) {
        x = (ticks.tms_stime - _recent_sys) + (ticks.tms_utime - _recent_usr);
        x /= (now - _recent_ticks);
        x /= cpu_count;
        x *= 100;
    } else {
        //Overflow detection. Just skip this value.
        x = -1.0;
    }

    _recent_ticks = now;
    _recent_sys = ticks.tms_stime;
    _recent_usr = ticks.tms_utime;

    return x;
}

bool process_times_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    error err;
    auto cpu_usage = calculate_cpu_usage(& err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return false;
    }

    if (cpu_usage < 0.0)
        return true;

    if (f != nullptr)
        f("cpu_usage", counter_t{cpu_usage}, user_data_ptr);

    return true;
}

}} // namespace ionic::metrics

