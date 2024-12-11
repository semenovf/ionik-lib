////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/error.hpp"
#include "pfs/ionik/namespace.hpp"
#include <string>

IONIK__NAMESPACE_BEGIN

namespace metrics {

class freedesktop_provider
{
private:
    std::string _os_name;
    std::string _os_pretty_name;

public:
    freedesktop_provider (error * perr = nullptr);

public:
    std::string os_name () const
    {
        return _os_name;
    }

    std::string os_pretty_name () const
    {
        return _os_pretty_name;
    }

};

} // namespace metrics

IONIK__NAMESPACE_END
