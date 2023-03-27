################################################################################
# Copyright (c) 2022 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2022.08.21 Initial version
################################################################################
project(usb_monitor)

add_executable(${PROJECT_NAME} main.cpp)

if (TARGET pfs::ionik)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)
elseif (TARGET pfs::ionik::static)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()

