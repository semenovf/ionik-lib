####################################################################################################
# Copyright (c) 2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2024.03.06 Initial version.
####################################################################################################
project(available_video_capture_devices C CXX)

portable_target(ADD_EXECUTABLE ${PROJECT_NAME})
portable_target(SOURCES ${PROJECT_NAME} main.cpp)

if (TARGET pfs::ionik)
    portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::ionik)
elseif (TARGET pfs::ionik::static)
    portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()


