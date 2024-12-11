////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "counter.hpp"
#include "pfs/ionik/error.hpp"
#include "pfs/ionik/namespace.hpp"
#include <pfs/string_view.hpp>

IONIK__NAMESPACE_BEGIN

namespace metrics {

// See man getrusage(2)

class getrusage_provider
{
public:
    using string_view = pfs::string_view;

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
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);

};

} // namespace metrics

IONIK__NAMESPACE_END
