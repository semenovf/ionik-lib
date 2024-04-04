################################################################################
# Copyright (c) 2023,2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2023.02.15 Initial version (netty-lib).
#      2024.04.04 Moved from `netty-lib`.
################################################################################
project(iface-monitor-demo)

add_executable(${PROJECT_NAME} main.cpp)

if (TARGET pfs::ionik)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()
