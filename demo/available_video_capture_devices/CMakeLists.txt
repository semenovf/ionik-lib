####################################################################################################
# Copyright (c) 2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#       2024.03.06 Initial version.
#       2024.11.23 Removed `portable_target` dependency.
####################################################################################################
project(available_video_capture_devices C CXX)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)


