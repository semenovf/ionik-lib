////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.12.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/namespace.hpp"
#include <cstdlib>
#include <string>

IONIK__NAMESPACE_BEGIN

namespace metrics {

struct os_info
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
    std::string device_name; ///< Device (hostname for Linux) name, e.g. "DESKTOP-XYZABCD"
    std::string cpu_vendor;  ///< Processor vendor, e.g. "GenuineIntel"
    std::string cpu_brand;   ///< Processor description, e.g. "Intel(R) Core(TM) i5-9600K CPU @ 3.70GHz 3.70 GHz"
    double ram_installed;    ///< Installed RAM in megabytes
    // std::uint8_t os_bits;    ///< Operation system bits, e.g. 32 or 64
    // std::uint8_t cpu_bits;   ///< CPU system bits, e.g. 32 or 64

    // Kernel (Linux specific)
    std::string sysname;        ///< Operating system name, e.g. "Linux" (Linux only)
    std::string kernel_release; ///< Kernel release, e.g. "6.8.0-49-generic" (Linux only)
    std::string machine;        ///< Hardware identifier, e.g. x86_64 (Linux only)
};

} // namespace metrics

IONIK__NAMESPACE_END

