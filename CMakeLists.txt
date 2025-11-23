################################################################################
# Copyright (c) 2022-2025 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#       2022.08.21 Initial version
#       2024.09.04 Set CMAKE_RUNTIME_OUTPUT_DIRECTORY.
#       2024.11.23 Up to C++14 standard.
#                  Removed `portable_target` dependency.
#       2025.11.09 Merged with library.cmake.
################################################################################
cmake_minimum_required (VERSION 3.19)
project(ionik CXX C)

include(CheckIncludeFile)

option(IONIK__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(IONIK__BUILD_TESTS "Build tests" OFF)
option(IONIK__BUILD_DEMO "Build examples/demo" OFF)

option(IONIK__BUILD_STATIC "Force build static library" OFF)
option(IONIK__ENABLE_QT5 "Enable Qt5 Multimedia as backend" OFF)
option(IONIK__ENABLE_QT6 "Enable Qt6 Multimedia as backend (NOT IMPLEMENTED YET)" OFF)
option(IONIK__DISABLE_FETCH_CONTENT "Disable fetch content if sources of dependencies already exists in the working tree (checks .git subdirectory)" ON)
option(IONIK__ENABLE_AGGRESSIVE_COMPILE_CHECK "Use aggressive check compile options (g++ only)" OFF)

# Used by Win32 service specific projects /MACHINE:{ARM|ARM64|ARM64EC|EBC|X64|X86}
# https://learn.microsoft.com/en-us/cpp/build/reference/machine-specify-target-platform?view=msvc-170
if (NOT IONIK__SERVICE_PLATFORM)
    set(IONIK__SERVICE_PLATFORM "X64" CACHE STRING "")
endif()

if (IONIK__BUILD_STRICT)
    if (NOT CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 14)
    endif()

    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

add_subdirectory(2ndparty)

####################################################################################################
# library specific block
####################################################################################################
set(_ionik__audio_backend_FOUND OFF)

if (BUILD_SHARED_LIBS AND NOT IONIK__BUILD_STATIC)
    add_library(ionik)
    target_compile_definitions(ionik PUBLIC IONIK__EXPORTS)
else()
    add_library(ionik STATIC)
    target_compile_definitions(ionik PUBLIC IONIK__STATIC)
endif()

add_library(pfs::ionik ALIAS ionik)

if (MSVC)
    target_compile_definitions(ionik PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_sources(ionik PRIVATE
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
        ${CMAKE_CURRENT_LIST_DIR}/src/metrics/linuxinfo_provider.cpp
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

            target_compile_definitions(ionik PUBLIC IONIK__DEVICE_OBSERVER_DISABLED=1)
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

if (IONIK__ENABLE_QT6)
    message(WARNING " Qt6 support for audio backend not implemented yet")
elseif (IONIK__ENABLE_QT5)
    # On Ubuntu (all Debian-based?) need to install `libgl-dev` package
    find_package(Qt5 COMPONENTS Core Gui Network Multimedia REQUIRED)
    target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/audio/device_info_qt5.cpp)
    target_link_libraries(ionik PRIVATE Qt5::Core Qt5::Gui Qt5::Network Qt5::Multimedia)
    set(_ionik__audio_backend_FOUND ON)
endif()

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
    target_sources(ionik PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/audio/device_info_dumb.cpp)
endif()

target_include_directories(ionik
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs/ionik
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs)
target_link_libraries(ionik PUBLIC pfs::common)

if (IONIK__ENABLE_AGGRESSIVE_COMPILE_CHECK)
    include(AggressiveCheckOpts)
    set(_is_sanitize_thread FALSE)
    aggressive_check_opts(ionik ${_is_sanitize_thread})
endif()

####################################################################################################

if (IONIK__BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

if (IONIK__BUILD_DEMO AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/demo)
    add_subdirectory(demo)
endif()

include(GNUInstallDirs)

install(TARGETS ionik
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
