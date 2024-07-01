################################################################################
# Copyright (c) 2024 Vladislav Trifochkin
#
# This file is part of `ionic-lib`.
#
# Changelog:
#      2024.06.30 Initial version.
################################################################################
project(filesystem_monitor)

add_executable(${PROJECT_NAME} main.cpp)

if (TARGET pfs::ionik)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)
elseif (TARGET pfs::ionik::static)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()
