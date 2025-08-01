################################################################################
# Copyright (c) 2022-2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#       2022.08.21 Initial version
#       2024.09.04 Set CMAKE_RUNTIME_OUTPUT_DIRECTORY.
#       2024.11.23 Up to C++14 standard.
#                  Removed `portable_target` dependency.
################################################################################
cmake_minimum_required (VERSION 3.19)
project(ionik-ALL CXX)

option(IONIK__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(IONIK__BUILD_TESTS "Build tests" OFF)
option(IONIK__BUILD_DEMO "Build examples/demo" OFF)

option(IONIK__BUILD_SHARED "Enable build shared library" OFF)
option(IONIK__ENABLE_QT5 "Enable Qt5 Multimedia as backend" OFF)
option(IONIK__ENABLE_QT6 "Enable Qt6 Multimedia as backend (NOT IMPLEMENTED YET)" OFF)
option(IONIK__DISABLE_FETCH_CONTENT "Disable fetch content if sources of dependencies already exists in the working tree (checks .git subdirectory)" ON)

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

get_property(_runtime_output_directory GLOBAL PROPERTY RUNTIME_OUTPUT_DIRECTORY)
get_property(_library_output_directory GLOBAL PROPERTY LIBRARY_OUTPUT_DIRECTORY)

if (NOT _runtime_output_directory)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
endif()

if (NOT _library_output_directory)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
endif()

add_subdirectory(2ndparty)
include(${CMAKE_CURRENT_LIST_DIR}/library.cmake)

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
