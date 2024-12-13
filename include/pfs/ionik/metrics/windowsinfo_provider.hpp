////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/error.hpp"
#include "pfs/ionik/namespace.hpp"
#include <cstdlib>
#include <string>
#include <vector>

IONIK__NAMESPACE_BEGIN

namespace metrics {

class windowsinfo_provider
{
public:
    struct os_release_record
    {
        // Freedesktop compatible 
        std::string name;
        std::string pretty_name;
        std::string version;
        std::string version_id;
        std::string codename;
        std::string id;
        std::string id_like;

        // Device specification
        std::string device_name; ///< Device (computer) name, e.g. "DESKTOP-XYZABCD"
        std::string cpu_vendor;  ///< Processor vendor, e.g. "GenuineIntel"
        std::string cpu_brand;   ///< Processor description, e.g. "Intel(R) Core(TM) i5-9600K CPU @ 3.70GHz 3.70 GHz"
        double ram_installed;    ///< Installed RAM in megabytes
        std::uint8_t os_bits;    ///< Operation system bits, e.g. 32 or 64
        std::uint8_t cpu_bits;   ///< CPU system bits, e.g. 32 or 64
    };

private:
    os_release_record _os_release;

public:
    windowsinfo_provider (error * perr = nullptr);

public:
    os_release_record const & os_release () const noexcept
    {
        return _os_release;
    }
};

} // namespace metrics

IONIK__NAMESPACE_END
