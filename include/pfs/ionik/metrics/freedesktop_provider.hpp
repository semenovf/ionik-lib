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
public:
    struct os_release_record
    {
        std::string name;
        std::string pretty_name;
        std::string version;
        std::string version_id;
        std::string codename;
        std::string id;
        std::string id_like;
    };

private:
    os_release_record _os_release;

public:
    freedesktop_provider (error * perr = nullptr);

public:
    os_release_record const & os_release () const noexcept
    {
        return _os_release;
    }
};

} // namespace metrics

IONIK__NAMESPACE_END
