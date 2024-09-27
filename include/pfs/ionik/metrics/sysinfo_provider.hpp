////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
//      2024.09.24 Changed `sysinfo_provider::query()` signature.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include <pfs/string_view.hpp>
#include <pfs/ionik/error.hpp>

namespace ionik {
namespace metrics {

class sysinfo_provider
{
public:
    using string_view = pfs::string_view;

public:
    sysinfo_provider ();

public:
    /**
     * Supported keys:
     *      * uptime    - seconds since boot;
     *      * totalram  - total usable main memory size, in bytes;
     *      * freeram   - available memory size, in bytes;
     *      * sharedram - amount of shared memory, in bytes;
     *      * bufferram - memory used by buffers, in bytes;
     *      * totalswap - total swap space size, in bytes;
     *      * freeswap  - swap space still available, in bytes;
     *      * totalhigh - total high memory size, in bytes;
     *      * freehigh  - available high memory size, in bytes.
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics
