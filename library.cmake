################################################################################
# Copyright (c) 2022 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2022.08.21 Initial version.
#      2023.03.27 Separated static and shared builds.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(ionik CXX)

option(IONIK__BUILD_SHARED "Enable build shared library" OFF)
option(IONIK__BUILD_STATIC "Enable build static library" ON)

if (NOT PORTABLE_TARGET__CURRENT_PROJECT_DIR)
    set(PORTABLE_TARGET__CURRENT_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if (IONIK__BUILD_SHARED)
    portable_target(ADD_SHARED ${PROJECT_NAME} ALIAS pfs::ionik EXPORTS IONIK__EXPORTS)
endif()

if (IONIK__BUILD_STATIC)
    set(STATIC_PROJECT_NAME ${PROJECT_NAME}-static)
    portable_target(ADD_STATIC ${STATIC_PROJECT_NAME} ALIAS pfs::ionik::static EXPORTS IONIK__STATIC)
endif()

if (PFS__LOG_LEVEL)
    list(APPEND _ionik__definitions "PFS__LOG_LEVEL=${PFS__LOG_LEVEL}")
endif()

list(APPEND _ionik__sources
    ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/local_file_provider.cpp)

list(APPEND _ionik__include_dirs ${CMAKE_CURRENT_LIST_DIR}/include)

if (ANDROID)
    list(APPEND _ionik__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_android.cpp)
elseif (UNIX)
    list(APPEND _ionik__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_libudev.cpp)

    list(APPEND _ionik__private_libs udev)

    if (NOT EXISTS /usr/include/libudev.h)
        list(APPEND _ionik__include_dirs ${CMAKE_CURRENT_LIST_DIR}/src/libudev1)
    endif()
elseif (MSVC)
    list(APPEND _ionik__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_win32.cpp)

    list(APPEND _ionik__private_libs Setupapi)
else()
    message (FATAL_ERROR "Unsupported platform")
endif()

if (NOT TARGET pfs::common)
    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/common/library.cmake)
endif()

if (IONIK__BUILD_SHARED)
    portable_target(SOURCES ${PROJECT_NAME} ${_ionik__sources})
    portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${_ionik__include_dirs})
    portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::common)

    if (_ionik__definitions)
        portable_target(DEFINITIONS ${PROJECT_NAME} PUBLIC ${_ionik__definitions})
    endif()

    if (_ionik__private_libs)
        portable_target(LINK ${PROJECT_NAME} PRIVATE ${_ionik__private_libs})
    endif()
endif()

if (IONIK__BUILD_STATIC)
    portable_target(SOURCES ${STATIC_PROJECT_NAME} ${_ionik__sources})
    portable_target(INCLUDE_DIRS ${STATIC_PROJECT_NAME} PUBLIC ${_ionik__include_dirs})
    portable_target(LINK ${STATIC_PROJECT_NAME} PUBLIC pfs::common)

    if (_ionik__definitions)
        portable_target(DEFINITIONS ${STATIC_PROJECT_NAME} PUBLIC ${_ionik__definitions})
    endif()

    if (_ionik__private_libs)
        portable_target(LINK ${STATIC_PROJECT_NAME} PRIVATE ${_ionik__private_libs})
    endif()
endif()
