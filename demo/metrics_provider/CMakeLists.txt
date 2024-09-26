################################################################################
# Copyright (c) 2024 Vladislav Trifochkin
#
# This file is part of `ionic-lib`.
#
# Changelog:
#      2024.09.11 Initial version.
################################################################################
project(metrics_provider)

add_executable(${PROJECT_NAME} main.cpp)

if (TARGET pfs::ionik)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)
elseif (TARGET pfs::ionik::static)
    target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()

if (MSVC)
    add_executable(pdh_browse_counters pdh_browse_counters.cpp)
    target_compile_definitions(pdh_browse_counters PRIVATE UNICODE "utf-8")
endif()
