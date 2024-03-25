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
#include <cstdint>
#include <map>
#include <ratio>
#include <string>
#include <utility>
#include <vector>

namespace ionik {
namespace video {

enum class subsystem_enum {
      video4linux2
    , camera2android
};

struct frame_size
{
    std::uint32_t width;  // Frame width in pixels
    std::uint32_t height; // Frame height in pixels
};

struct frame_rate
{
    // FPS = denom / num
    std::uint32_t num;
    std::uint32_t denom;

#if __ANDROID__
    std::uint32_t min_num;
    std::uint32_t min_denom;
#endif
};

struct discrete_frame_size: frame_size
{
    std::vector<frame_rate> frame_rates;
};

struct pixel_format
{
    std::string name; // 4-characters string (e.g. `YUYV`, `GRBG` etc)
    std::string description;
    std::vector<discrete_frame_size> discrete_frame_sizes;
};

struct capture_device_info
{
    subsystem_enum subsystem;

    // For video4linux2 subsystem contains the path to the device in the file system.
    // For camera2android subsystem contains Camera ID.
    std::string id;
    std::string readable_name;

    // Camera orientation
    int orientation;

    // For video4linux2 subsystem contains:
    //      * driver  - name of the driver module;
    //      * card    - name of the card (equivalent to readable_name);
    //      * path    - path to the device in the file system;
    //      * bus     - name of the bus;
    //      * version - kernel version.
    // For camera2android subsystem contains:
    //      * backward_compatible - "1" (true) or "0" (false);
    //      * facing  - "0" (front), "1" (back) or "2" (external).

    std::map<std::string, std::string> data;

    std::vector<pixel_format> pixel_formats;
    std::size_t current_pixel_format_index;
    frame_size current_frame_size;
};

IONIK__EXPORT std::vector<capture_device_info> fetch_capture_devices (error * perr = nullptr);

}} // namespace ionik::video
