////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "os_info.hpp"
#include "pfs/ionik/error.hpp"
#include "pfs/ionik/namespace.hpp"
#include <string>

IONIK__NAMESPACE_BEGIN

namespace metrics {

class linuxinfo_provider
{
private:
    os_info _os_info;

public:
    linuxinfo_provider (error * perr = nullptr);

public:
    os_info const & get_info () const noexcept
    {
        return _os_info;
    }
};

} // namespace metrics

IONIK__NAMESPACE_END
