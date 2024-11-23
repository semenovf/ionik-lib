################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2023.12.24 Initial version.
#      2024.11.23 Removed `portable_target` dependency.
################################################################################
project(already_running)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)
