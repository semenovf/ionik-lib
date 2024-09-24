////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/sysinfo_provider.hpp"
#include "pfs/numeric_cast.hpp"
#include <sys/sysinfo.h>

namespace ionik {
namespace metrics {

sysinfo_provider::sysinfo_provider () = default;

bool sysinfo_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    struct sysinfo si;

    auto rc = sysinfo(& si);

    if (rc != 0) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , pfs::system_error_text()
        });

        return false;
    }

    if (f != nullptr) {
        (void)(!f("uptime", counter_t{pfs::numeric_cast<std::int64_t>(si.uptime)}, user_data_ptr)
            && !f("totalram", counter_t{pfs::numeric_cast<std::int64_t>(si.totalram)}, user_data_ptr)
            && !f("freeram", counter_t{pfs::numeric_cast<std::int64_t>(si.freeram)}, user_data_ptr)
            && !f("sharedram", counter_t{pfs::numeric_cast<std::int64_t>(si.sharedram)}, user_data_ptr)
            && !f("bufferram", counter_t{pfs::numeric_cast<std::int64_t>(si.bufferram)}, user_data_ptr)
            && !f("totalswap", counter_t{pfs::numeric_cast<std::int64_t>(si.totalswap)}, user_data_ptr)
            && !f("freeswap", counter_t{pfs::numeric_cast<std::int64_t>(si.freeswap)}, user_data_ptr)
            && !f("totalhigh", counter_t{pfs::numeric_cast<std::int64_t>(si.totalhigh)}, user_data_ptr)
            && !f("freehigh", counter_t{pfs::numeric_cast<std::int64_t>(si.freehigh)}, user_data_ptr));
    }

    return true;
}

}} // namespace ionic::metrics

