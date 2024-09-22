////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "parser.hpp"
#include "ionik/metrics/proc_provider.hpp"
#include "ionik/local_file.hpp"
#include <pfs/i18n.hpp>
#include <pfs/numeric_cast.hpp>
#include <cctype>

namespace ionik {
namespace metrics {

using string_view = proc_meminfo_provider::string_view;

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
        && advance_decimal_digits_value(p, last, rec.value)
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
proc_meminfo_provider::parse (error * perr)
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