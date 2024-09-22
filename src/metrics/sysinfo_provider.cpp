////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/sysinfo_provider.hpp"
#include <pfs/log.hpp>
#include <sys/sysinfo.h>

namespace ionik {
namespace metrics {

sysinfo_provider::sysinfo_provider () = default;

bool sysinfo_provider::query (error * perr)
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

    LOGD("--"
        , "Total RAM: {:.2f} Gb\n"
            "Free  RAM: {:.2f} Mb\n"
        , static_cast<double>(si.totalram) / (1000 * 1000 * 1000)
        , static_cast<double>(si.freeram) / (1024 * 1024));
    return true;
}

}} // namespace ionic::metrics

