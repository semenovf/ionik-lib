################################################################################
# Copyright (c) 2022 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2022.08.21 Initial version.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(ionik CXX)

portable_target(ADD_SHARED ${PROJECT_NAME} ALIAS pfs::ionik
    EXPORTS IONIK__EXPORTS
    BIND_STATIC ${PROJECT_NAME}-static
    STATIC_ALIAS pfs::ionik::static
    STATIC_EXPORTS IONIK__STATIC)

if (UNIX)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_libudev.cpp)
elseif (MSVC)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/device_observer_win32.cpp)
else()
    message (FATAL_ERROR "Unsupported platform")
endif()

if (NOT TARGET pfs::common)
    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/common/library.cmake)
endif()

portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
portable_target(INCLUDE_DIRS ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs/ionik)
portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::common)
portable_target(LINK ${PROJECT_NAME}-static PUBLIC pfs::common)

if (UNIX)
    portable_target(LINK ${PROJECT_NAME} PRIVATE udev)
    portable_target(LINK ${PROJECT_NAME}-static PRIVATE udev)
elseif (MSVC)
#    portable_target(COMPILE_OPTIONS ${PROJECT_NAME} "/wd4251")
#    portable_target(COMPILE_OPTIONS ${PROJECT_NAME}-static "/wd4251")
   portable_target(LINK ${PROJECT_NAME} PRIVATE Setupapi)
   portable_target(LINK ${PROJECT_NAME}-static PRIVATE Setupapi)
endif(UNIX)
