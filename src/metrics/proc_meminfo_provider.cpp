////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/proc_provider.hpp"
#include "ionik/local_file.hpp"
#include <pfs/i18n.hpp>
#include <pfs/numeric_cast.hpp>
#include <cctype>

namespace ionik {
namespace metrics {

using string_view = proc_meminfo_provider::string_view;

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

inline bool advance_key (std::string::const_iterator & pos, std::string::const_iterator last
    , string_view & key)
{
    auto p = pos;

    if (!advance_word(p, last))
        return false;

    key = string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

inline bool advance_value (std::string::const_iterator & pos, std::string::const_iterator last
    , string_view & value)
{
    auto p = pos;

    if (!advance_decimal_digits(p, last))
        return false;

    value = string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

inline bool advance_units (std::string::const_iterator & pos, std::string::const_iterator last
    , string_view & units)
{
    auto p = pos;

    if (!advance_word(p, last))
        return true;

    units = pfs::string_view{pos.base(), static_cast<std::size_t>(p - pos)};
    pos = p;
    return true;
}

bool proc_meminfo_provider::parse_record (std::string::const_iterator & pos
    , std::string::const_iterator last, record_view & rec, error * perr)
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
        && advance_ws(p, last)
        && advance_value(p, last, rec.value)
        && advance_ws(p, last)
        && advance_units(p, last, rec.units)
        && advance_ws(p, last)
        && advance_nl(p, last);

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

std::map<string_view, proc_meminfo_provider::record_view>
proc_meminfo_provider::parse_rv (error * perr)
{
    if (!read_all(perr))
        return std::map<string_view, record_view>{};

    auto pos = _content.cbegin();
    auto last = _content.cend();

    record_view rec;
    std::map<string_view, record_view> result;

    while (parse_record(pos, last, rec))
        result.emplace(rec.key, rec);

    return result;
}

std::map<std::string, proc_meminfo_provider::record>
proc_meminfo_provider::parse (error * perr)
{
    using pfs::to_string;

    if (!read_all(perr))
        return std::map<std::string, record>{};

    auto pos = _content.cbegin();
    auto last = _content.cend();

    record_view rec;
    std::map<std::string, record> result;

    while (parse_record(pos, last, rec))
        result.emplace(to_string(rec.key), record {
              to_string(rec.key)
            , to_string(rec.value)
            , to_string(rec.units)
        });

    return result;
}

bool proc_meminfo_provider::read_all (error * perr)
{
    error err;
    proc_reader reader {PFS__LITERAL_PATH("/proc/meminfo"), & err};

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return false;
    }

    _content = reader.content();
    return true;
}

}} // namespace ionik::metrics
