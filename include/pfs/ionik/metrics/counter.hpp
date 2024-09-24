////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/ionik/error.hpp>
#include <pfs/string_view.hpp>
#include <pfs/variant.hpp>
#include <cmath>
#include <cstdint>

namespace ionik {
namespace metrics {

using counter_t = pfs::variant<std::int64_t, double>;

inline double to_double (counter_t const & c)
{
    if (pfs::holds_alternative<double>(c))
        return pfs::get<double>(c);
    else if (pfs::holds_alternative<std::int64_t>(c))
        return static_cast<double>(pfs::get<std::int64_t>(c));

    return static_cast<double>(pfs::get<0>(c));
}

inline std::int64_t to_integer (counter_t const & c)
{
    if (pfs::holds_alternative<std::int64_t>(c))
        return pfs::get<std::int64_t>(c);
    else if (pfs::holds_alternative<double>(c))
        return static_cast<std::int64_t>(std::round(pfs::get<double>(c)));

    return static_cast<std::int64_t>(pfs::get<0>(c));
}

counter_t to_int64_counter (pfs::string_view value, pfs::string_view units, error * perr = nullptr);

}} // namespace ionik::metrics
