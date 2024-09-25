////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/string_view.hpp>
#include <string>

namespace ionik {
namespace metrics {

inline bool is_ws (char ch)
{
    return ch == ' ' || ch == '\t';
}

inline bool is_nl (char ch)
{
    return ch == '\n';
}

inline void skip_ws (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last)
{
    while (pos != last && is_ws(*pos))
        ++pos;
}

bool advance_ws (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last);
bool advance_nl (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last);
bool advance_until_nl (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last);
bool advance_word (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last);
bool advance_colon (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last);
bool advance_decimal_digits (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last);
bool advance_key (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last, pfs::string_view & key);
bool advance_decimal_digits_value (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last, pfs::string_view & value);
bool advance_unparsed_value (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last, pfs::string_view & value);
bool advance_units (pfs::string_view::const_iterator & pos, pfs::string_view::const_iterator last, pfs::string_view & units);

}} // namespace ionik::metrics

