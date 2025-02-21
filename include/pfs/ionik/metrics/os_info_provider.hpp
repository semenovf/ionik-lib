////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#if _MSC_VER
#   include "windowsinfo_provider.hpp"
#elif __linux__
#   include "linuxinfo_provider.hpp"
#else
#   error "Unsupported operation system for OS provider"
#endif

IONIK__NAMESPACE_BEGIN

namespace metrics {

class os_info_provider
{
private:
#if _MSC_VER
    windowsinfo_provider _d;
#elif __linux__
    linuxinfo_provider _d;
#endif

public:
    os_info_provider (error * perr = nullptr)
        : _d(perr)
    {}

public:
    os_info const & get_info () const noexcept
    {
        return _d.get_info();
    }
};

} // namespace metrics

IONIK__NAMESPACE_END
