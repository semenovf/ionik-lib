////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2022.08.21 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/error.hpp"
#include "exports.hpp"

namespace ionik {

////////////////////////////////////////////////////////////////////////////////
// Error codes, category, exception class
////////////////////////////////////////////////////////////////////////////////
using error_code = std::error_code;

enum class errc
{
      success = 0
    , acquire_device_observer
    , operation_system_error
    , bad_data_format
    , backend_error
    , unsupported
};

class error_category : public std::error_category
{
public:
    IONIK__EXPORT char const * name () const noexcept override;
    IONIK__EXPORT std::string message (int ev) const override;
};

inline std::error_category const & get_error_category ()
{
    static error_category instance;
    return instance;
}

inline std::error_code make_error_code (errc e)
{
    return std::error_code(static_cast<int>(e), get_error_category());
}

inline std::system_error make_exception (errc e)
{
    return std::system_error(make_error_code(e));
}

class error: public pfs::error
{
public:
    error () : pfs::error() {}

    error (errc ec)
        : pfs::error(make_error_code(ec))
    {}

    error (errc ec
        , std::string const & description
        , std::string const & cause)
        : pfs::error(make_error_code(ec), description, cause)
    {}

    error (errc ec
        , std::string const & description)
        : pfs::error(make_error_code(ec), description)
    {}

    using pfs::error::error;
};

} // namespace ionik
