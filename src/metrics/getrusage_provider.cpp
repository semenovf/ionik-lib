////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/getrusage_provider.hpp"
#include <sys/time.h>
#include <sys/resource.h>

namespace ionik {
namespace metrics {

getrusage_provider::getrusage_provider () = default;

bool getrusage_provider::query (bool (* f) (string_view key, long value), error * perr)
{
    struct rusage r;
    int rc = getrusage(RUSAGE_SELF, & r);

    if (rc != 0) {
        pfs::throw_or(perr, error {
              pfs::get_last_system_error()
            , pfs::system_error_text()
        });

        return false;
    }

    if (f != nullptr) {
        (void)(!f("maxrss", r.ru_maxrss)
            && !f("ixrss", r.ru_ixrss)
            && !f("idrss", r.ru_idrss)
            && !f("isrss", r.ru_isrss));
    }

    return true;
}

}} // namespace ionic::metrics
