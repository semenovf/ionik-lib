################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2021.06.18 Initial version (netty-lib).
#      2024.04.03 Moved from `netty-lib`.
################################################################################
project(mtu)

add_executable(${PROJECT_NAME} main.cpp)

if (TARGET pfs::ionik)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()
