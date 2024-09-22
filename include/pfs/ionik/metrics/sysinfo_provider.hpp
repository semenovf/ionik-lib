////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
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
     *      * totalram  - total usable main memory size;
     *      * freeram   - integral unshared data size;
     *      * sharedram - amount of shared memory;
     *      * bufferram - memory used by buffers;
     *      * totalswap - total swap space size;
     *      * freeswap  - swap space still available;
     *      * totalhigh - total high memory size;
     *      * freehigh  - available high memory size.
     */
    bool query (bool (* f) (string_view key, unsigned long value), error * perr = nullptr);
};

}} // namespace ionik::metrics
