////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/proc_meminfo_provider.hpp"
#include "parser.hpp"
#include "proc_reader.hpp"
#include "ionik/local_file.hpp"
#include <pfs/i18n.hpp>
#include <pfs/numeric_cast.hpp>
#include <cctype>

// References:
// 1. [/proc/meminfo](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/6/html/deployment_guide/s2-proc-meminfo)
// 2. [Interpreting /proc/meminfo and free output for Red Hat Enterprise Linux](https://access.redhat.com/solutions/406773)

namespace ionik {
namespace metrics {

using string_view = proc_meminfo_provider::string_view;

proc_meminfo_provider::proc_meminfo_provider () = default;

bool proc_meminfo_provider::parse_record (string_view::const_iterator & pos
    , string_view::const_iterator last, record_view & rec, error * perr)
{
    auto p = pos;

    if (p == last)
        return false;

    skip_ws(p, last);

    if (p == last)
        return false;

    rec.key = string_view{};
    rec.value = string_view{};
    rec.units = string_view{};

    auto success = advance_key(p, last, rec.key)
        && advance_colon(p, last)
        && advance_ws0n(p, last)
        && advance_decimal_digits_value(p, last, rec.value)
        && advance_ws0n(p, last)
        && advance_units(p, last, rec.units)
        && advance_ws0n(p, last)
        && advance_nl1n(p, last);

    if (!success) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_data
            , tr::_("unexpected meminfo record format")
        });

        return false;
    }

    if (rec.key.empty()) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_data
            , tr::_("meminfo record key is empty")
        });

        return false;
    }

    if (rec.value.empty()) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_data
            , tr::_("meminfo record value is empty")
        });

        return false;
    }

    pos = p;
    return true;
}

bool proc_meminfo_provider::read_all (error * perr)
{
    error err;
    proc_reader reader {PFS__LITERAL_PATH("/proc/meminfo"), & err};

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return false;
    }

    _content = reader.move_content();
    return true;
}

bool proc_meminfo_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    if (f == nullptr)
        return true;

    if (!read_all(perr))
        return false;

    string_view content_view {_content};
    auto pos = content_view.cbegin();
    auto last = content_view.cend();

    record_view rec;

    while (parse_record(pos, last, rec)) {
        bool is_counter_format1 = rec.key == "MemTotal"
            || rec.key == "MemFree"
            || rec.key == "Cached"
            || rec.key == "SwapCached"
            || rec.key == "SwapTotal"
            || rec.key == "SwapFree";

        if (is_counter_format1) {
            auto c = to_int64_counter(rec.value, rec.units);

            if (f(rec.key, c, user_data_ptr))
                break;
        }
    }

    return true;
}

}} // namespace ionik::metrics
