////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/string_view.hpp>
#include <pfs/ionik/error.hpp>

namespace ionik {
namespace metrics {

// GlobalMemoryStatusEx provider

class gms_provider
{
public:
    using string_view = pfs::string_view;

public:
    gms_provider ();

public:
    /**
      * Supported keys:
      *     * MemoryLoad    - percents of memory in use;
      *     * TotalPhys     - total physical memory in bytes;
      *     * AvailPhys     - free physical memory in bytes;
      *     * TotalPageFile - total of paging file in bytes;
      *     * AvailPageFile - free of paging file in bytes;
      *     * TotalVirtual  - total of virtual memory in bytes;
      *     * AvailVirtual  - free of virtual memory in bytes;
      *     * AvailExtendedVirtual - free of extended memory in bytes;
      */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics
