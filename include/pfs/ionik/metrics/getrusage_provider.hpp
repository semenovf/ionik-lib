////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/string_view.hpp>
#include <pfs/ionik/error.hpp>

namespace ionik {
namespace metrics {

// See man getrusage(2)

class getrusage_provider
{
public:
    using string_view = pfs::string_view;

    struct record_view
    {
        string_view key;
        long value;
    };

public:
    getrusage_provider ();

public:
    /**
     * Supported keys:
     *      * maxrss - maximum resident set size;
     *      * ixrss  - integral shared memory size;
     *      * idrss  - integral unshared data size;
     *      * isrss  - integral unshared stack size.
     */
    bool query (bool (* f) (string_view key, long value) , error * perr = nullptr);
};

}} // namespace ionik::metrics
