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

namespace ionik {

////////////////////////////////////////////////////////////////////////////////
// Error codes, category, exception class
////////////////////////////////////////////////////////////////////////////////
using error_code = std::error_code;

enum class errc
{
      success = 0
    , acquire_device_observer
};

class error_category : public std::error_category
{
public:
    virtual char const * name () const noexcept override
    {
        return "pfs::ionik";
    }

    virtual std::string message (int ev) const override
    {
        switch (ev) {
            case static_cast<int>(errc::success):
                return std::string{"no error"};
            case static_cast<int>(errc::acquire_device_observer):
                return std::string{"acquire device observer"};
            default: return std::string{"unknown net error"};
        }
    }
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

} // namespace ionik
