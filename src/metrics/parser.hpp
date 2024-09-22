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

inline void skip_ws (std::string::const_iterator & pos, std::string::const_iterator last)
{
    while (pos != last && is_ws(*pos))
        ++pos;
}

bool advance_ws (std::string::const_iterator & pos, std::string::const_iterator last);
bool advance_nl (std::string::const_iterator & pos, std::string::const_iterator last);
bool advance_until_nl (std::string::const_iterator & pos, std::string::const_iterator last);
bool advance_word (std::string::const_iterator & pos, std::string::const_iterator last);
bool advance_colon (std::string::const_iterator & pos, std::string::const_iterator last);
bool advance_decimal_digits (std::string::const_iterator & pos, std::string::const_iterator last);
bool advance_key (std::string::const_iterator & pos, std::string::const_iterator last, pfs::string_view & key);
bool advance_decimal_digits_value (std::string::const_iterator & pos, std::string::const_iterator last, pfs::string_view & value);
bool advance_unparsed_value (std::string::const_iterator & pos, std::string::const_iterator last, pfs::string_view & value);
bool advance_units (std::string::const_iterator & pos, std::string::const_iterator last, pfs::string_view & units);

}} // namespace ionik::metrics

