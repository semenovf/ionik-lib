################################################################################
# Copyright (c) 2022 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2022.08.21 Initial version.
#      2023.03.27 Separated static and shared builds.
#      2023.03.31 Added audio devices info support (from obsolete `multimedia-lib`).
################################################################################
cmake_minimum_required (VERSION 3.11)
project(ionik CXX)

option(IONIK__BUILD_SHARED "Enable build shared library" OFF)
option(IONIK__BUILD_STATIC "Enable build static library" ON)

option(IONIK__ENABLE_PULSEAUDIO "Enable PulseAudio as backend" ON)
option(IONIK__ENABLE_QT5 "Enable Qt5 Multimedia as backend" OFF)

set(_ionik__audio_backend_FOUND OFF)

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
    ${CMAKE_CURRENT_LIST_DIR}/src/local_file_provider.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/audio/wav_explorer.cpp)

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
        ${CMAKE_CURRENT_LIST_DIR}/src/video/capture_device_info_v4l2.cpp)

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

if (NOT _ionik__audio_backend_FOUND)
    message(FATAL_ERROR
        " No any Audio backend found\n"
        " For Debian-based distributions it may be PulseAudio ('libpulse-dev' package)")
endif()

if (NOT TARGET pfs::common)
    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/2ndparty/common/library.cmake)
endif()

list(REMOVE_DUPLICATES _ionik__sources)
list(REMOVE_DUPLICATES _ionik__private_libs)
list(REMOVE_DUPLICATES _ionik__include_dirs)
list(REMOVE_DUPLICATES _ionik__definitions)

if (IONIK__BUILD_SHARED)
    portable_target(SOURCES ${PROJECT_NAME} ${_ionik__sources})
    portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${_ionik__include_dirs})
    portable_target(INCLUDE_DIRS ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs/ionik)
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
    portable_target(INCLUDE_DIRS ${STATIC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs/ionik)
    portable_target(LINK ${STATIC_PROJECT_NAME} PUBLIC pfs::common)

    if (_ionik__definitions)
        portable_target(DEFINITIONS ${STATIC_PROJECT_NAME} PUBLIC ${_ionik__definitions})
    endif()

    if (_ionik__private_libs)
        portable_target(LINK ${STATIC_PROJECT_NAME} PRIVATE ${_ionik__private_libs})
    endif()
endif()
