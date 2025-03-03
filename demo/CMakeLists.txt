####################################################################################################
# Copyright (c) 2022-2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2022.08.21 Initial version.
#      2023.03.31 Added `available_audio_devices` demo (from obsolete `multimedia-lib`).
#      2023.10.19 Added `audio_spectrum` demo.
#      2023.12.24 Added `already_running` demo.
#      2024.04.03 `mtu` and `iface-monitor` moved from `netty-lib`.
#      2024.04.04 Added `available_net_interfaces` demo.
#      2024.06.30 Added `filesystem_monitor` demo.
####################################################################################################
add_subdirectory(already_running)
add_subdirectory(audio_spectrum)
add_subdirectory(available_audio_devices)
add_subdirectory(available_video_capture_devices)
add_subdirectory(filesystem_monitor)
add_subdirectory(metrics_provider)
add_subdirectory(usb_monitor)

if (MSVC)
    add_subdirectory(win32_service)
endif()
