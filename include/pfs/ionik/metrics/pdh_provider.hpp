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
#include <pdh.h>

namespace ionik {
namespace metrics {

// MS Windows Performance Data Helper provider

class pdh_provider
{
public:
    using string_view = pfs::string_view;

private:
    PDH_HQUERY _hquery {INVALID_HANDLE_VALUE};
    PDH_HCOUNTER _processor_time {INVALID_HANDLE_VALUE};
    //PDH_HCOUNTER _mem_available {INVALID_HANDLE_VALUE};

public:
    pdh_provider (error * perr = nullptr);
    ~pdh_provider ();

public:
    /**
     * Supported keys:
     *      * ProcessorTime - total cpu utilization;
     */
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);
};

}} // namespace ionik::metrics
