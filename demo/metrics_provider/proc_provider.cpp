////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "proc_provider.hpp"
#include "ionik/local_file.hpp"
#include <pfs/numeric_cast.hpp>
#include <cctype>

#include <pfs/log.hpp>

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

inline bool advance_ws (std::string::const_iterator & pos, std::string::const_iterator last)
{
    if (pos == last)
        return false;

    if (!is_ws(*pos))
        return true;

    ++pos;

    skip_ws(pos, last);

    return true;
}

inline bool advance_nl (std::string::const_iterator & pos, std::string::const_iterator last)
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


inline bool advance_word (std::string::const_iterator & pos, std::string::const_iterator last)
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

inline bool advance_colon (std::string::const_iterator & pos, std::string::const_iterator last)
{
    if (pos == last)
        return false;

    if (*pos != ':')
        return false;

    ++pos;

    return true;
}

inline bool advance_decimal_digits (std::string::const_iterator & pos, std::string::const_iterator last)
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// proc_reader
////////////////////////////////////////////////////////////////////////////////////////////////////
proc_reader::proc_reader (pfs::filesystem::path const & path, error * perr)
{
    auto proc_file = local_file::open_read_only(path, perr);

    if (!proc_file)
        return;


    _content = proc_file.read_all(perr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// proc_meminfo_provider
////////////////////////////////////////////////////////////////////////////////////////////////////
proc_meminfo_provider::proc_meminfo_provider () = default;

struct meminfo_parse_context
{
    pfs::string_view key;
    pfs::string_view value;
    pfs::string_view units;
};

inline bool advance_key (std::string::const_iterator & pos, std::string::const_iterator last
    , meminfo_parse_context & ctx)
{
    auto p = pos;

    if (!advance_word(p, last))
        return false;

    ctx.key = pfs::string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

inline bool advance_value (std::string::const_iterator & pos, std::string::const_iterator last
    , meminfo_parse_context & ctx)
{
    auto p = pos;

    if (!advance_decimal_digits(p, last))
        return false;

    ctx.value = pfs::string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

inline bool advance_units (std::string::const_iterator & pos, std::string::const_iterator last
    , meminfo_parse_context & ctx)
{
    auto p = pos;

    if (!advance_word(p, last))
        return true;

    ctx.units = pfs::string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

bool proc_meminfo_provider::parse_key_value (std::string::const_iterator & pos, std::string::const_iterator last)
{
    auto p = pos;

    if (p == last)
        return false;

    skip_ws(p, last);

    if (p == last)
        return false;

    meminfo_parse_context ctx;

    auto success = advance_key(p, last, ctx)
        && advance_colon(p, last)
        && advance_ws(p, last)
        && advance_value(p, last, ctx)
        && advance_ws(p, last)
        && advance_units(p, last, ctx)
        && advance_ws(p, last)
        && advance_nl(p, last);

    if (!success) {
        LOGE("", "PARSE FAILURE");
        return false;
    }

    if (ctx.key.empty()) {
        LOGE("", "KEY IS EMPTY");
        return false;
    }

    if (ctx.value.empty()) {
        LOGE("", "VALUE IS EMPTY");
        return false;
    }

    using pfs::to_string;
    LOGD("", "{}: {} {}", to_string(ctx.key), to_string(ctx.value), to_string(ctx.units));

    pos = p;
    return true;
}

bool proc_meminfo_provider::parse (error * perr)
{
    auto pos = _content.cbegin();
    auto last = _content.cend();

    while (parse_key_value(pos, last))
        ;

    return true;
}

bool proc_meminfo_provider::query (error * perr)
{
    error err;
    proc_reader reader {PFS__LITERAL_PATH("/proc/meminfo"), & err};

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return false;
    }

    _content = reader.content();
    return parse(perr);
}

}} // namespace ionik::metrics
