################################################################################
# Copyright (c) 2025 Vladislav Trifochkin
#
# This file is part of `netty-lib`.
#
# Changelog:
#       2025.04.13 Initial version
################################################################################

if (NOT TARGET pfs::common)
    if (NETTY__DISABLE_FETCH_CONTENT AND EXISTS common/.git)
        add_subdirectory(common)
    else()
        set(FETCHCONTENT_UPDATES_DISCONNECTED_COMMON ON)
        message(STATUS "Fetching common ...")
        include(FetchContent)
        FetchContent_Declare(common
            GIT_REPOSITORY https://github.com/semenovf/common-lib.git
            GIT_TAG master
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
            # SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/2ndparty/common)
        FetchContent_MakeAvailable(common)
        message(STATUS "Fetching common complete")
    endif()
endif()
