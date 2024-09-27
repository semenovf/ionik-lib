////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/gms_provider.hpp"
#include <pfs/numeric_cast.hpp>
#include <pfs/windows.hpp>
#include <cstdint>

namespace ionik {
namespace metrics {

gms_provider::gms_provider () = default;

bool gms_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    if (f == nullptr)
        return true;

    // Retrieves information about the system's current usage of both physical and virtual memory.

    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(& mem_info);

    (void)(!f("MemoryLoad"   , counter_t {pfs::numeric_cast<std::int64_t>(mem_info.dwMemoryLoad)}, user_data_ptr)
        && !f("TotalPhys"    , counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullTotalPhys)}, user_data_ptr)
        && !f("AvailPhys"    , counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullAvailPhys)}, user_data_ptr)
        && !f("TotalPageFile", counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullTotalPageFile)}, user_data_ptr)
        && !f("AvailPageFile", counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullAvailPageFile)}, user_data_ptr)
        && !f("TotalVirtual" , counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullTotalVirtual)}, user_data_ptr)
        && !f("AvailVirtual" , counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullAvailVirtual)}, user_data_ptr)
        && !f("AvailExtendedVirtual", counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullAvailExtendedVirtual)}, user_data_ptr)
        && !f("TotalSwap", counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullTotalPageFile - mem_info.ullTotalPhys)}, user_data_ptr)
        && !f("AvailSwap", counter_t {pfs::numeric_cast<std::int64_t>(mem_info.ullAvailPageFile - mem_info.ullAvailPhys)}, user_data_ptr));


    return true;
}

}} // namespace ionic::metrics
