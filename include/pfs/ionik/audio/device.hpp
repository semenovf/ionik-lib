////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file was part of `multimedia-lib` until 2023.03.31.
// This file is part of `ionik-lib`.
//
// Changelog:
//      2021.08.03 Initial version (multimedia-lib).
//      2021.08.21 default_input_device, default_output_device, fetch_devices for PulseAudio.
//      2022.01.20 Added Qt5 backend.
//      2023.03.31 Initial version (ionik-lib).
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/exports.hpp"
#include <string>
#include <vector>

namespace ionik {
namespace audio {

enum class device_mode
{
      output  = 0x01
    , input   = 0x02
};

struct device_info
{
    std::string name;
    std::string readable_name;
};

/**
 * Checks whether this library can retrieve information about audio devices.
 */
IONIK__EXPORT bool supported ();

/**
 * Default source/input device.
 */
IONIK__EXPORT device_info default_input_device ();

/**
 * Default sink/output device.
 */
IONIK__EXPORT device_info default_output_device ();

/**
 * Fetches all available audio devices according to @a mode.
 */
IONIK__EXPORT std::vector<device_info> fetch_devices (device_mode mode);

}} // namespace ionik::audio
