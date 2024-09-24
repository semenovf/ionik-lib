
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/string_view.hpp>
#include <pfs/ionik/error.hpp>

namespace ionik {
namespace metrics {

// GetProcessMemoryInfo/GetPerformanceInfo provider

class psapi_provider
{
public:
    using string_view = pfs::string_view;

public:
    psapi_provider ();

public:
    /**
     * Supported keys:
     * a) Provided by GetProcessMemoryInfo:
     *      * PrivateUsage       - virtual memory currently used by current process, in bytes (the total amount of private memory that the memory manager has committed for a running process);
     *      * WorkingSetSize     - physical memory currently used by current process, in bytes;
     *      * PeakWorkingSetSize - the peak physical memory used by current process, in bytes.
     * b) Provided by GetPerformanceInfo:
     *      * PhysicalTotal      - the amount of actual physical memory, in bytes;
     *      * PhysicalAvailable  - the amount of physical memory currently available, in bytes;
     *      * SystemCache        - the amount of system cache memory, in bytes;
     *      * HandleCount        - the current number of open handles;
     *      * ProcessCount       - the current number of processes;
     *      * ThreadCount        - the current number of threads.
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics


