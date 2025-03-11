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

bool getrusage_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr)
{
    struct rusage r;
    int rc = getrusage(RUSAGE_SELF, & r);

    if (rc != 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), pfs::system_error_text());
        return false;
    }

    if (f != nullptr) {
        (void)(!f("maxrss", counter_t{static_cast<std::int64_t>(r.ru_maxrss)}, user_data_ptr)
            && !f("ixrss", counter_t{static_cast<std::int64_t>(r.ru_ixrss)}, user_data_ptr)
            && !f("idrss", counter_t{static_cast<std::int64_t>(r.ru_idrss)}, user_data_ptr)
            && !f("isrss", counter_t{static_cast<std::int64_t>(r.ru_isrss)}, user_data_ptr));
    }

    return true;
}

}} // namespace ionic::metrics
