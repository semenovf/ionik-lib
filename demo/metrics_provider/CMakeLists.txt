################################################################################
# Copyright (c) 2024 Vladislav Trifochkin
#
# This file is part of `ionic-lib`.
#
# Changelog:
#       2024.09.11 Initial version.
################################################################################
project(metrics_provider)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)

if (MSVC)
    add_executable(pdh_browse_counters pdh_browse_counters.cpp)
    target_compile_definitions(pdh_browse_counters PRIVATE UNICODE _UNICODE)
    target_compile_options(pdh_browse_counters PRIVATE "/utf-8")
endif()
