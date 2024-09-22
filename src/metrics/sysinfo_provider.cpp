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

bool sysinfo_provider::query (bool (* f) (string_view key, unsigned long value) , error * perr)
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
        (void)(!f("uptime", pfs::numeric_cast<unsigned long>(si.uptime))
            && !f("totalram", si.totalram)
            && !f("freeram", si.freeram)
            && !f("sharedram", si.sharedram)
            && !f("bufferram", si.bufferram)
            && !f("totalswap", si.totalswap)
            && !f("freeswap", si.freeswap)
            && !f("totalhigh", si.totalhigh)
            && !f("freehigh", si.freehigh));
    }

    return true;
}

}} // namespace ionic::metrics

