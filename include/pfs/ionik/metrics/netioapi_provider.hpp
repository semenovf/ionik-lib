////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.10.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_net_provider.hpp"
#include <pfs/ionik/exports.hpp>
#include <wtypes.h>
#include <string>

namespace ionik {
namespace metrics {

class netioapi_provider: public basic_net_provider
{
private:
    int _if_index {0};
    std::string _alias;
    std::string _desc;

private:
    bool read_all (error * perr);

public:
    IONIK__EXPORT netioapi_provider (int if_index, error * perr = nullptr);
    IONIK__EXPORT netioapi_provider (std::string const & alias, error * perr = nullptr);

    ~netioapi_provider () = default;
    netioapi_provider (netioapi_provider &&) = default;
    netioapi_provider & operator = (netioapi_provider &&) = default;

    netioapi_provider (netioapi_provider const &) = delete;
    netioapi_provider & operator = (netioapi_provider const &) = delete;

public:
    IONIK__EXPORT std::string const & iface_name () const noexcept;
    IONIK__EXPORT std::string const & readable_name () const noexcept;

    /**
    * Supported keys:
    *      * rx_bytes - received bytes totally (std::uint64_t);
    *      * tx_bytes - transferred bytes totally (std::uint64_t);
    *      * rx_speed - receive speed, in bytes per second (double);
    *      * tx_speed - transfer speed, in bytes per second (double).
    *      * rx_speed_max - max receive speed, in bytes per second (double);
    *      * tx_speed_max - max transfer speed, in bytes per second (double).
    *
    * @return @c false on failure.
    */
    IONIK__EXPORT bool query (bool (* f) (string_view key, counter_t const & value, void * user_data_ptr)
        , void * user_data_ptr, error * perr = nullptr);

    IONIK__EXPORT bool query (counter_group & counters, error * perr = nullptr);

public: // static
    IONIK__EXPORT static std::vector<std::string> interfaces (error * perr = nullptr);
};

}} // namespace ionik::metrics
