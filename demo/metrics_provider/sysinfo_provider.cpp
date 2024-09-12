////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "sysinfo_provider.hpp"
#include <pfs/bits/operationsystem.h>
#include <pfs/log.hpp>

#if PFS__OS_LINUX
#   include <sys/sysinfo.h>
#endif

namespace ionik {
namespace metrics {

sysinfo_provider::sysinfo_provider ()
{
#if PFS__OS_LINUX
#else
    throw pfs::error {ionik::errc::unsupported, tr::f_("only Linux supported `sysinfo`")}
#endif
}

bool sysinfo_provider::query (error * perr)
{
#if PFS__OS_LINUX
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
#else
    return false;
#endif
}

}} // namespace ionic::metrics

