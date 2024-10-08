////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/metrics/random_metrics_provider.hpp"
#include <random>

namespace ionik {
namespace metrics {

using random_engine_t = std::mt19937;

inline auto engine () -> random_engine_t
{
    static std::random_device __rd; // Will be used to obtain a seed for the random number engine
    return random_engine_t{__rd()}; // Standard mersenne_twister_engine seeded with rd()
}

std::int64_t random_int64 (std::int64_t from, std::int64_t to)
{
    if (from == to)
        return from;

    std::uniform_int_distribution<std::int64_t> distrib{from, to};
    auto rnd = engine();
    return distrib(rnd);
}

double random_double (std::int64_t from, std::int64_t to, int precision)
{
    if (from == to)
        return from;

    if (precision > 6)
        precision = 6;

    std::int64_t max_denom = 1;

    while (precision--)
        max_denom *= 10;

    std::uniform_int_distribution<std::int64_t> integral_part{from, to};
    std::uniform_int_distribution<std::int64_t> fractional_part{0, max_denom};

    auto rnd = engine();

    return static_cast<double>(integral_part(rnd)) + 1 / static_cast<double>(fractional_part(rnd));
}

random_metrics_provider::random_metrics_provider ()
    : _ml(metric_limits{})
{}

random_metrics_provider::random_metrics_provider (metric_limits && ml)
    : _ml(std::move(ml))
{}

bool random_metrics_provider::query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
    , void * user_data_ptr, error * /*perr*/)
{
    if (f != nullptr) {
        auto cpu_usage_total = random_double(_ml.cpu_usage_total_range.first
            , _ml.cpu_usage_total_range.second, _ml.precision);
        auto cpu_usage = random_double(_ml.cpu_usage_range.first
            , _ml.cpu_usage_range.second, _ml.precision);
        auto ram_free = (_ml.ram_total * random_int64(_ml.ram_free_range.first, _ml.ram_free_range.second)) / 100;
        auto swap_free = (_ml.swap_total * random_int64(_ml.swap_free_range.first, _ml.swap_free_range.second)) / 100;
        auto mem_usage = (_ml.ram_total * random_int64(_ml.mem_usage.first, _ml.mem_usage.second)) / 100;

        (void)(!f("cpu_usage_total", cpu_usage_total, user_data_ptr)
            && !f("cpu_usage", cpu_usage, user_data_ptr)
            && !f("ram_total", _ml.ram_total, user_data_ptr)
            && !f("ram_free", ram_free, user_data_ptr)
            && !f("swap_total", _ml.swap_total, user_data_ptr)
            && !f("swap_free", swap_free, user_data_ptr)
            && !f("mem_usage", mem_usage, user_data_ptr));
    }

    return true;
}

}} // namespace ionik::metrics



