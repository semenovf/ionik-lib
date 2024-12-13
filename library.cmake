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

set(_ionik__audio_backend_FOUND OFF)

if (IONIK__BUILD_SHARED)
    add_library(ionik SHARED)
    target_compile_definitions(ionik PRIVATE IONIK__EXPORTS)
else()
    add_library(ionik STATIC)
    target_compile_definitions(ionik PUBLIC IONIK__STATIC)
endif()

add_library(pfs::ionik ALIAS ionik)

if (MSVC)
    target_compile_definitions(ionik PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_sources(ionik PRIVATE
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
    target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/already_running.cpp)
endif()

if (ANDROID)
    target_sources(ionik PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_android.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/video/capture_device_info_android.cpp)

    target_link_libraries(ionik PRIVATE camera2ndk)
elseif (UNIX)
    target_sources(ionik PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/freedesktop_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/getrusage_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/parser.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/proc_meminfo_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/proc_reader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/proc_self_status_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/proc_stat_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/sys_class_net_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/sysinfo_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/video/capture_device_info_v4l2.cpp)

    check_include_file(libudev.h _has_libudev_h)

    if (_has_libudev_h)
        target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_libudev.cpp)
        target_link_libraries(ionik PRIVATE udev)
    else()
        message(WARNING
            "No 'udev' library found\n"
            "For Debian-based distributions try to install 'libudev-dev' package"
            "Device observer based on 'udev' library disabled")

        #if (NOT EXISTS /usr/include/libudev.h)
        #    target_include_directories(ionik PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/libudev1)
        #endif()
    endif()

elseif (MSVC)
    target_sources(ionik PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_win32.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/pdh_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/gms_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/netioapi_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/psapi_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/windowsinfo_provider.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/video/capture_device_info_win.cpp)

    target_compile_options(ionik PRIVATE "/wd4251" "/wd4267" "/wd4244")
    target_link_libraries(ionik PRIVATE Setupapi
        Mf Mfplat Mfreadwrite Mfuuid # Media Foundation library
        Pdh iphlpapi)
else()
    message (FATAL_ERROR "Unsupported platform")
endif()

if (IONIK__ENABLE_QT5)
    find_package(Qt5 COMPONENTS Core Gui Network Multimedia REQUIRED)
    target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/audio/device_info_qt5.cpp)
    target_link_directories(ionik PRIVATE Qt5::Core Qt5::Gui Qt5::Network Qt5::Multimedia)
    set(_ionik__audio_backend_FOUND ON)
endif(IONIK__ENABLE_QT5)

if (NOT _ionik__audio_backend_FOUND)
    #if (UNIX AND NOT APPLE AND NOT CYGWIN)
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        # In Ubuntu it is a part of 'libpulse-dev' package
        find_package(PulseAudio)

        if (PULSEAUDIO_FOUND)
            message(STATUS "PulseAudio version: ${PULSEAUDIO_VERSION}")

            target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/audio/device_info_pulseaudio.cpp)

            target_include_directories(ionik PUBLIC ${PULSEAUDIO_INCLUDE_DIR})
            target_link_libraries(ionik PRIVATE ${PULSEAUDIO_LIBRARY})
            set(_ionik__audio_backend_FOUND ON)
        endif()
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/audio/device_info_win32.cpp)
        set(_ionik__audio_backend_FOUND ON)
    endif()
endif()

if (UNIX)
    check_include_file(sys/inotify.h _ionik__has_sys_inotify_h)

    if (_ionik__has_sys_inotify_h)
        target_compile_definitions(ionik PUBLIC "IONIK__HAS_INOTIFY=1")
        target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/filesystem_monitor/inotify.cpp)
    else()
        message(FATAL_ERROR "inotify NOT FOUND")
    endif()
endif(UNIX)

if (MSVC)
    target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/filesystem_monitor/win32.cpp)
endif(MSVC)

if (NOT _ionik__audio_backend_FOUND)
    message(WARNING
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

target_include_directories(ionik
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs/ionik
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs)
target_link_libraries(ionik PUBLIC pfs::common)
