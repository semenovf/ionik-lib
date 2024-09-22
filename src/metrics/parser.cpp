////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "parser.hpp"
#include <cctype>

namespace ionik {
namespace metrics {

using string_view = pfs::string_view;

bool advance_ws (std::string::const_iterator & pos, std::string::const_iterator last)
{
    if (pos == last)
        return false;

    if (!is_ws(*pos))
        return true;

    ++pos;

    skip_ws(pos, last);

    return true;
}

bool advance_nl (std::string::const_iterator & pos, std::string::const_iterator last)
{
    if (pos == last)
        return false;

    if (*pos != '\n')
        return false;

    ++pos;

    while (pos != last && *pos == '\n')
        ++pos;

    return true;
}

bool advance_until_nl (std::string::const_iterator & pos, std::string::const_iterator last)
{
    if (pos == last)
        return false;

    while (pos != last && *pos != '\n')
        ++pos;

    return true;
}

bool advance_word (std::string::const_iterator & pos, std::string::const_iterator last)
{
    if (pos == last)
        return false;

    if (!::isalpha(*pos))
        return false;

    ++pos;

    while (pos != last && (::isalnum(*pos) || *pos == '(' || *pos == ')' || *pos == '_'))
        ++pos;

    return true;
}

bool advance_colon (std::string::const_iterator & pos, std::string::const_iterator last)
{
    if (pos == last)
        return false;

    if (*pos != ':')
        return false;

    ++pos;

    return true;
}

bool advance_decimal_digits (std::string::const_iterator & pos, std::string::const_iterator last)
{
    if (pos == last)
        return false;

    if (!::isdigit(*pos))
        return false;

    ++pos;

    while (pos != last && ::isdigit(*pos))
        ++pos;

    return true;
}

bool advance_key (std::string::const_iterator & pos, std::string::const_iterator last, string_view & key)
{
    auto p = pos;

    if (!advance_word(p, last))
        return false;

    key = string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

bool advance_decimal_digits_value (std::string::const_iterator & pos, std::string::const_iterator last
    , string_view & value)
{
    auto p = pos;

    if (!advance_decimal_digits(p, last))
        return false;

    value = string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

bool advance_unparsed_value (std::string::const_iterator & pos, std::string::const_iterator last, pfs::string_view & value)
{
    auto p = pos;

    if (!advance_until_nl(p, last))
        return false;

    value = string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

bool advance_units (std::string::const_iterator & pos, std::string::const_iterator last
    , string_view & units)
{
    auto p = pos;

    if (!advance_word(p, last))
        return true;

    units = pfs::string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

}} // namespace ionik::metrics
