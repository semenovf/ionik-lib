////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.03.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "video/capture_device.hpp"

namespace ionik {
namespace video {

#ifndef __ANDROID__

std::vector<capture_device_info>
sanitize_capture_devices (std::vector<capture_device_info> const & devices)
{
    std::vector<capture_device_info> good_devices;

    for (auto const & dev: devices) {
        if (!dev.pixel_formats.empty())
            good_devices.push_back(dev);
    }

    return good_devices;
}

#endif

}} // namespace ionik::video
