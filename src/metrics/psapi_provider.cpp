////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/psapi_provider.hpp"
#include <pfs/numeric_cast.hpp>
#include <pfs/windows.hpp>
#include <psapi.h>
#include <cstdint>

namespace ionik {
namespace metrics {

psapi_provider::psapi_provider () = default;

bool psapi_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    if (f == nullptr)
        return true;

    // Retrieves information about the system's current usage of both physical and virtual memory.

    PROCESS_MEMORY_COUNTERS_EX pmc;
    pmc.cb = sizeof(pmc);
    auto rc = GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *) & pmc, sizeof(pmc));

    if (rc == 0) {
        pfs::throw_or(perr, pfs::get_last_system_error(), "GetProcessMemoryInfo failure");
        return false;
    }

    auto can_continue = (!f("PrivateUsage", counter_t{ pfs::numeric_cast<std::int64_t>(pmc.PrivateUsage) }, user_data_ptr)
        && !f("WorkingSetSize", counter_t{ pfs::numeric_cast<std::int64_t>(pmc.WorkingSetSize) }, user_data_ptr)
        && !f("PeakWorkingSetSize", counter_t{ pfs::numeric_cast<std::int64_t>(pmc.PeakWorkingSetSize) }, user_data_ptr));

    if (!can_continue)
        return true;

    PERFORMANCE_INFORMATION perf_info;
    perf_info.cb = sizeof(perf_info);
    auto success = GetPerformanceInfo(& perf_info, perf_info.cb);

    if (success != TRUE) {
        pfs::throw_or(perr, error{
              pfs::get_last_system_error()
            , "GetPerformanceInfo failure"
        });

        return false;
    }

    auto page_size = pfs::numeric_cast<std::int64_t>(perf_info.PageSize);

    (void) (!f("PhysicalTotal", counter_t{ pfs::numeric_cast<std::int64_t>(perf_info.PhysicalTotal) * page_size}, user_data_ptr)
        && !f("PhysicalAvailable", counter_t{ pfs::numeric_cast<std::int64_t>(perf_info.PhysicalAvailable) * page_size }, user_data_ptr)
        && !f("SystemCache", counter_t{ pfs::numeric_cast<std::int64_t>(perf_info.SystemCache) * page_size }, user_data_ptr)
        && !f("HandleCount", counter_t{ pfs::numeric_cast<std::int64_t>(perf_info.HandleCount)}, user_data_ptr)
        && !f("ProcessCount", counter_t{ pfs::numeric_cast<std::int64_t>(perf_info.ProcessCount) }, user_data_ptr)
        && !f("ThreadCount", counter_t{ pfs::numeric_cast<std::int64_t>(perf_info.ThreadCount) }, user_data_ptr));

    return true;
}

}} // namespace ionic::metrics
