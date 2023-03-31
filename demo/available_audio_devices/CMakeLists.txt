################################################################################
# Copyright (c) 2021-2023 Vladislav Trifochkin
#
# This file is part of `multimedia-lib`.
#
# Changelog:
#      2021.08.03 Initial version.
################################################################################
project(available_audio_devices)

add_executable(${PROJECT_NAME} main.cpp)

if (TARGET pfs::ionik)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)
elseif (TARGET pfs::ionik::static)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()
