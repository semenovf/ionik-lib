////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Vladislav Trifochkin
//
// This file was part of `multimedia-lib` until 2023.03.31.
// This file is part of `ionik-lib`.
//
// Changelog:
//      2025.07.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ionik/audio/device.hpp"

namespace ionik {
namespace audio {

bool supported ()
{
    return false;
}

device_info default_input_device ()
{
    device_info result;
    return result;
}
device_info default_output_device ()
{
    device_info result;
    return result;
}

std::vector<device_info> fetch_devices (device_mode)
{
    return std::vector<device_info>{};
}

}} // namespace ionik::audio
