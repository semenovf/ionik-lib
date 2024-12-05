################################################################################
# Copyright (c) 2022-2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#       2022.08.21 Initial version.
#       2023.03.27 Separated static and shared builds.
#       2023.03.31 Added audio devices info support (from obsolete `multimedia-lib`).
#       2024.11.23 Removed `portable_target` dependency.
################################################################################
cmake_minimum_required (VERSION 3.19)
project(ionik CXX C)

include(CheckIncludeFile)

option(IONIK__BUILD_SHARED "Enable build shared library" OFF)
option(IONIK__ENABLE_PULSEAUDIO "Enable PulseAudio as backend" ON)
option(IONIK__ENABLE_QT5 "Enable Qt5 Multimedia as backend" OFF)

set(_ionik__audio_backend_FOUND OFF)

if (IONIK__BUILD_SHARED)
    add_library(ionik SHARED)
    target_compile_definitions(ionik PRIVATE IONIK__EXPORTS)
else()
    add_library(ionik STATIC)
    target_compile_definitions(ionik PRIVATE IONIK__STATIC)
endif()

add_library(pfs::ionik ALIAS ionik)

if (MSVC)
    target_compile_definitions(ionik PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

list(APPEND _ionik__sources
    ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/local_file_provider.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/audio/wav_explorer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/metrics/counter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/metrics/network_counters.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/metrics/random_counters.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/metrics/random_metrics_provider.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/metrics/system_counters.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/metrics/times_provider.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/video/capture_device_info.cpp)

if (NOT ANDROID)
    list(APPEND _ionik__sources ${CMAKE_CURRENT_LIST_DIR}/src/already_running.cpp)
endif()

list(APPEND _ionik__include_dirs ${CMAKE_CURRENT_LIST_DIR}/include)

if (ANDROID)
    list(APPEND _ionik__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_android.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/video/capture_device_info_android.cpp)

    list(APPEND _ionik__private_libs camera2ndk)
elseif (UNIX)
    list(APPEND _ionik__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_libudev.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/getrusage_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/parser.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/proc_meminfo_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/proc_reader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/proc_self_status_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/proc_stat_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/sys_class_net_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/sysinfo_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/video/capture_device_info_v4l2.cpp)

    list(APPEND _ionik__private_libs udev)

    if (NOT EXISTS /usr/include/libudev.h)
        list(APPEND _ionik__include_dirs ${CMAKE_CURRENT_LIST_DIR}/src/libudev1)
    endif()
elseif (MSVC)
    list(APPEND _ionik__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_win32.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/pdh_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/gms_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/netioapi_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/psapi_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/video/capture_device_info_win.cpp)

    list(APPEND _ionik__compile_options "/wd4251" "/wd4267" "/wd4244")
    list(APPEND _ionik__private_libs Setupapi
        Mf Mfplat Mfreadwrite Mfuuid # Media Foundation library
        Pdh iphlpapi)
else()
    message (FATAL_ERROR "Unsupported platform")
endif()

if (IONIK__ENABLE_QT5)
    list(APPEND _ionik__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/audio/device_info_qt5.cpp)

    if (IONIK__BUILD_SHARED)
        portable_target(LINK_QT5_COMPONENTS ${PROJECT_NAME} PRIVATE Core Gui Network Multimedia)
    endif()

    if (IONIK__BUILD_STATIC)
        portable_target(LINK_QT5_COMPONENTS ${STATIC_PROJECT_NAME} PRIVATE Core Gui Network Multimedia)
    endif()

    set(_ionik__audio_backend_FOUND ON)
endif(IONIK__ENABLE_QT5)

if (NOT _ionik__audio_backend_FOUND)
    #if (UNIX AND NOT APPLE AND NOT CYGWIN)
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        if (IONIK__ENABLE_PULSEAUDIO)
            # In Ubuntu it is a part of 'libpulse-dev' package
            find_package(PulseAudio)

            if (PULSEAUDIO_FOUND)
                message(STATUS "PulseAudio version: ${PULSEAUDIO_VERSION}")

                list(APPEND _ionik__sources
                    ${CMAKE_CURRENT_LIST_DIR}/src/audio/device_info_pulseaudio.cpp)

                list(APPEND _ionik__include_dirs ${PULSEAUDIO_INCLUDE_DIR})
                list(APPEND _ionik__private_libs ${PULSEAUDIO_LIBRARY})
                set(_ionik__audio_backend_FOUND ON)
            endif()
        endif()
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        list(APPEND _ionik__sources
            ${CMAKE_CURRENT_LIST_DIR}/src/audio/device_info_win32.cpp)
        set(_ionik__audio_backend_FOUND ON)
    endif()
endif()

if (UNIX)
    check_include_file(sys/inotify.h _ionik__has_sys_inotify_h)

    if (_ionik__has_sys_inotify_h)
        list(APPEND _ionik__definitions IONIK__HAS_INOTIFY=1)
        list(APPEND _ionik__sources ${CMAKE_CURRENT_LIST_DIR}/src/filesystem_monitor/inotify.cpp)
    else()
        message(FATAL_ERROR "inotify NOT FOUND")
    endif()
endif(UNIX)

if (MSVC)
    list(APPEND _ionik__sources ${CMAKE_CURRENT_LIST_DIR}/src/filesystem_monitor/win32.cpp)
endif(MSVC)

if (NOT _ionik__audio_backend_FOUND)
    message(FATAL_ERROR
        " No any Audio backend found\n"
        " For Debian-based distributions it may be PulseAudio ('libpulse-dev' package)")
endif()

if (NOT TARGET pfs::common)
    set(FETCHCONTENT_UPDATES_DISCONNECTED_COMMON ON)

    message(STATUS "Fetching pfs::common ...")
    include(FetchContent)
    FetchContent_Declare(common
        GIT_REPOSITORY https://github.com/semenovf/common-lib.git
        GIT_TAG master
        GIT_PROGRESS TRUE
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/2ndparty/common
        SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/2ndparty/common)
    FetchContent_MakeAvailable(common)
    message(STATUS "Fetching pfs::common complete")
endif()

list(REMOVE_DUPLICATES _ionik__sources)
list(REMOVE_DUPLICATES _ionik__private_libs)
list(REMOVE_DUPLICATES _ionik__include_dirs)
list(REMOVE_DUPLICATES _ionik__compile_options)

if (_ionik__definitions)
    list(REMOVE_DUPLICATES _ionik__definitions)
endif()

target_sources(ionik PRIVATE ${_ionik__sources})
target_include_directories(ionik
    PUBLIC ${_ionik__include_dirs}
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs/ionik
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs)
target_link_libraries(ionik PUBLIC pfs::common)

if (_ionik__compile_options)
    target_compile_options(ionik PRIVATE ${_ionik__compile_options})
endif()

if (_ionik__definitions)
    target_compile_definitions(ionik PUBLIC ${_ionik__definitions})
endif()

if (_ionik__private_libs)
    target_link_libraries(ionik PRIVATE ${_ionik__private_libs})
endif()
