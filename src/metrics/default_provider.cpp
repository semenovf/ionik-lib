////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.09.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/default_provider.hpp"
#include <pfs/optional.hpp>

#if _MSC_VER
// #   include "ionik/metrics/gms_provider.hpp"
// #   include "ionik/metrics/pdh_provider.hpp"
// #   include "ionik/metrics/psapi_provider.hpp"
#else
// #   include "ionik/metrics/proc_provider.hpp"
// #   include "ionik/metrics/sysinfo_provider.hpp"
#   include "ionik/metrics/times_provider.hpp"
// #   include "ionik/metrics/getrusage_provider.hpp"
#endif

namespace ionik {
namespace metrics {

using string_view = pfs::string_view;

struct counter_group
{
    pfs::optional<counter_t> cpu_usage;
};

class default_provider::impl
{
private:
#if _MSC_VER
    // TODO
#else
    // Provides CPU utilization by the current process
    times_provider _times_provider;
#endif

public:
    impl (error * perr)
        : _times_provider(perr)
    {}

public:
    bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , counter_group * pcounters, error * perr)
    {
#if _MSC_VER
        // TODO
        return true;
#else
        auto r1 = _times_provider.query([] (pfs::string_view key, counter_t const & value, void * pcounters) -> bool {
            if (key == "cpu_usage") {
                static_cast<counter_group *>(pcounters)->cpu_usage = value;
                return true;
            }

            return false;
        }, pcounters, perr);

        return r1;
#endif
    }
};

default_provider::default_provider (error * perr)
    : _d(new impl(perr))
{}


bool default_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * perr)
{
    counter_group counters;
    auto success = _d->query(f, & counters, perr);


    return true;
}

}} // namespace ionic::metrics
