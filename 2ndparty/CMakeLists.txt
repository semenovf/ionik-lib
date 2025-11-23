################################################################################
# Copyright (c) 2025 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#       2025.07.29 Initial version
################################################################################
include(FetchContent)

foreach (_dep common)
    if (NOT TARGET ${_dep})
        if (IONIK__DISABLE_FETCH_CONTENT AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/${_dep}/.git)
            add_subdirectory(${_dep})
        else()
            message(STATUS "Fetching ${_dep} ...")
            FetchContent_Declare(${_dep}
                GIT_REPOSITORY git@github.com:semenovf/${_dep}-lib.git
                GIT_TAG master
                SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/${_dep})
            FetchContent_MakeAvailable(${_dep})
            message(STATUS "Fetching ${_dep} complete")
        endif()
    endif()
endforeach()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/common/cmake" PARENT_SCOPE)
