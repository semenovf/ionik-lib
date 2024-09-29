////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/counter.hpp"
#include <pfs/i18n.hpp>
#include <pfs/integer.hpp>
#include <iterator>
#include <vector>

namespace ionik {
namespace metrics {

static std::int64_t units_to_bytes (pfs::string_view units, error * perr)
{
    if (units.empty())
        return 1;

    // /proc/meminfo units format
    //
    // [https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/6/html/deployment_guide/s2-proc-meminfo]
    // While the file shows kilobytes (kB; 1 kB equals 1000 B), it is actually kibibytes
    // (KiB; 1 KiB equals 1024 B). This imprecision in /proc/meminfo is known,
    // but is not corrected due to legacy concerns - programs rely on /proc/meminfo to specify
    // size with the "kB" string.
    if (units == "kB")
        return std::int64_t{1024};

    // /proc/cpuinfo units format
    if (units == "KB")
        return std::int64_t{1024};

    pfs::throw_or(perr, error {pfs::errc::unexpected_data, tr::f_("unsupported units: {}", units)});
    return 0;
}

counter_t to_int64_counter (pfs::string_view value, pfs::string_view units, error * perr)
{
    std::error_code ec;
    auto result = pfs::to_integer<std::int64_t>(value.cbegin(), value.cend(), 10, ec);

    if (ec) {
        pfs::throw_or(perr, error {ec, tr::f_("bad numeric value for: {}", value)});
        return counter_t{};
    }

    auto multiplier = units_to_bytes(units, perr);

    if (multiplier == 0)
        return counter_t{};

    result *= multiplier;

    return result;
}

}} // namespace ionik::metrics

