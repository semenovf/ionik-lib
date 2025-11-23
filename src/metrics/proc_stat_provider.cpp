////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "parser.hpp"
#include "proc_reader.hpp"
#include "ionik/metrics/proc_stat_provider.hpp"
#include <pfs/i18n.hpp>
#include <pfs/integer.hpp>
#include <pfs/numeric_cast.hpp>
#include <cstdint>
#include <iterator>
#include <thread>

namespace ionik {
namespace metrics {

using string_view = proc_stat_provider::string_view;

inline std::size_t cpu_core_index (string_view key)
{
    if (key == "cpu")
        return 0;

    auto index = pfs::to_integer<std::size_t>(key.cbegin() + 3, key.cend(), 10) + 1;

    if (index <= 0 || index > 4096) {
        throw error {tr::f_("too big number of CPU cores: {}", index)};
    }

    return index;
}

proc_stat_provider::proc_stat_provider (error * perr)
{
    auto const core_count = std::thread::hardware_concurrency();

    _cpu_recent_data.reserve(core_count + 1);

    if (!read_all(perr))
        return;

    string_view content_view {_content};
    auto pos  = content_view.cbegin();
    auto last = content_view.cend();

    record_view rec;

    while (parse_record(pos, last, rec, perr)) {
        if (pfs::starts_with(rec.key, "cpu")) {
            auto index = cpu_core_index(rec.key);

            auto opt_cpu_data = parse_cpu_data(rec);

            if (!opt_cpu_data) {
                pfs::throw_or(perr, tr::f_("bad value for `{}` in `/proc/stat`", rec.key));
                return;
            }

            _cpu_recent_data.resize(index + 1);
            _cpu_recent_data[index] = *opt_cpu_data;

            // LOGD("[stat]", "CPU {}: {} {} {} {}", index
            //     , _cpu_recent_data[index].usr
            //     , _cpu_recent_data[index].usr_low
            //     , _cpu_recent_data[index].sys
            //     , _cpu_recent_data[index].idl);
        }
    }
}

bool proc_stat_provider::parse_record (string_view::const_iterator & pos
    , string_view::const_iterator last, record_view & rec, error * perr)
{
    auto p = pos;

    if (p == last)
        return false;

    skip_ws(p, last);

    if (p == last)
        return false;

    rec.key   = string_view{};
    rec.values.clear();

    auto success = advance_key(p, last, rec.key);

    if (success) {
        auto tmp = p;

        while (advance_ws1n(p, last) && (tmp = p) && advance_token(tmp, last)) {
            rec.values.push_back(string_view{p, pfs::numeric_cast<std::size_t>(std::distance(p, tmp))});
            p = tmp;
        }

        advance_nl1n(p, last);
    }

    if (!success) {
        pfs::throw_or(perr, tr::_("unexpected `/proc/stat` record format"));
        return false;
    }

    if (rec.key.empty()) {
        pfs::throw_or(perr, tr::_("`/proc/stat` record key is empty"));
        return false;
    }

    if (rec.values.empty()) {
        pfs::throw_or(perr, tr::_("`/proc/stat` record value is empty"));
        return false;
    }

    pos = p;
    return true;
}

pfs::optional<proc_stat_provider::cpu_data>
proc_stat_provider::parse_cpu_data (record_view const & rec)
{
    bool is_valid = true;
    cpu_data result;

    if (rec.values.size() >= 4) {
        std::error_code ec0, ec1, ec2, ec3;
        result.usr     = pfs::to_integer<std::int64_t>(rec.values[0].cbegin(), rec.values[0].cend(), 10, ec0);
        result.usr_low = pfs::to_integer<std::int64_t>(rec.values[1].cbegin(), rec.values[1].cend(), 10, ec1);
        result.sys     = pfs::to_integer<std::int64_t>(rec.values[2].cbegin(), rec.values[2].cend(), 10, ec2);
        result.idl     = pfs::to_integer<std::int64_t>(rec.values[3].cbegin(), rec.values[3].cend(), 10, ec3);

        is_valid = !(ec0 || ec1 || ec2 || ec3);
    } else {
        is_valid = false;
    }

    if (!is_valid)
        return pfs::nullopt;

    return result;
}

bool proc_stat_provider::read_all (error * perr)
{
    error err;
    proc_reader reader {PFS__LITERAL_PATH("/proc/stat"), & err};

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return false;
    }

    _content = reader.move_content();
    return true;
}

pfs::optional<double> proc_stat_provider::calculate_cpu_usage (record_view const & rec)
{
    double result {-1};
    auto opt_cpu_data = parse_cpu_data(rec);

    if (!opt_cpu_data)
        return pfs::nullopt;

    auto index = cpu_core_index(rec.key);

    if (index >= _cpu_recent_data.size())
        return pfs::nullopt;

    auto & recent  = _cpu_recent_data[index];
    auto & current = *opt_cpu_data;

    auto overflow = current.usr < recent.usr || current.usr_low < recent.usr_low
        || current.sys < recent.sys || current.idl < recent.idl;

    if (overflow) {
        // Skip this value.
        result = double{-1};
    } else {
        std::int64_t total = (current.usr - recent.usr) + (current.usr_low - recent.usr_low)
            + (current.sys - recent.sys);
        result = static_cast<double>(total);
        total += (current.idl - recent.idl);

        if (total == 0) {
            // Skip this value.
            result = double{-1};
        } else {
            result /= static_cast<double>(total);
            result *= 100;
        }
    }

    recent = current;

    return result;
}

bool proc_stat_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    if (f == nullptr)
        return true;

    if (!read_all(perr))
        return false;

    string_view content_view {_content};
    auto pos  = content_view.cbegin();
    auto last = content_view.cend();

    record_view rec;

    while (parse_record(pos, last, rec, perr)) {
        if (pfs::starts_with(rec.key, "cpu")) {
            error err;
            auto opt_cpu_usage = calculate_cpu_usage(rec);

            if (!opt_cpu_usage) {
                pfs::throw_or(perr, tr::f_("bad value for `{}` in `/proc/stat`", rec.key));
                return false;
            }

            // Skip
            if (*opt_cpu_usage < double{0})
                continue;

            if (f(rec.key, counter_t{*opt_cpu_usage}, user_data_ptr))
                break;
        }
    }

    return true;
}

}} // namespace ionik::metrics
