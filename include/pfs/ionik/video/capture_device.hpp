////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.03.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/exports.hpp"
#include "pfs/ionik/error.hpp"
#include <string>
#include <map>
#include <utility>
#include <vector>

namespace ionik {
namespace video {

enum class subsystem_enum {
    video4linux2
};

struct capture_device_info
{
    subsystem_enum subsystem;
    std::string readable_name;

    // For video4linux2 subsystem contains:
    //      * driver    - name of the driver module;
    //      * card      - name of the card (equivalent to readable_name);
    //      * path      - path to the device in the file system;
    //      * bus       - name of the bus;
    //      * version   - kernel version.
    std::map<std::string, std::string> data;
};

IONIK__EXPORT std::vector<capture_device_info> fetch_capture_devices (error * perr = nullptr);

}} // namespace ionik::video
