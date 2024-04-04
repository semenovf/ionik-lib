////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.02.16 Initial version (netty-lib).
//      2024.04.03 Moved from `netty-lib`.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/error.hpp"
#include "pfs/ionik/exports.hpp"

#if IONIK__LIBMNL_ENABLED
struct mnl_socket;
#endif

namespace ionik {
namespace net {

/**
 * Netlink socket
 */
class netlink_socket
{
public:
    using native_type = int;
    static native_type constexpr kINVALID_SOCKET = -1;

    enum class type_enum {
          unknown = -1
        , route = 0 // NETLINK_ROUTE
    };

private:
#if IONIK__LIBMNL_ENABLED
    mnl_socket * _socket { nullptr };
#else
    native_type _socket { kINVALID_SOCKET };
#endif

public:
    /**
     * Constructs invalid Netlink socket.
     */
    netlink_socket ();

    /**
     * Constructs Netlink socket.
     */
    netlink_socket (type_enum netlinktype);

    netlink_socket (netlink_socket const &) = delete;
    netlink_socket & operator = (netlink_socket const &) = delete;

    IONIK__EXPORT ~netlink_socket ();

    IONIK__EXPORT netlink_socket (netlink_socket &&);
    IONIK__EXPORT netlink_socket & operator = (netlink_socket &&);

public:
    /**
     *  Checks if Netlink socket is valid
     */
    IONIK__EXPORT operator bool () const noexcept;

    IONIK__EXPORT native_type native () const noexcept;

    /**
     * Receive data from Netlink socket.
     */
    IONIK__EXPORT int recv (char * data, int len, error * perr = nullptr);

    /**
     * Send @a req request with @a len length on a Netlink socket.
     */
    IONIK__EXPORT int send (char const * req, int len, error * perr = nullptr);
};

}} // namespace ionik::net
