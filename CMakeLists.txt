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
option(IONIK__ENABLE_COVERAGE "Build tests with coverage support" OFF)
option(IONIK__ENABLE_PROFILER "Build with profiling support" OFF)

if (IONIK__BUILD_STRICT)
    if (NOT CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 14)
    endif()

    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

if (IONIK__BUILD_TESTS)
    set(IONIK__BUILD_SHARED ON)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

include(${CMAKE_CURRENT_LIST_DIR}/library.cmake)

if (IONIK__BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

if (IONIK__BUILD_DEMO AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/demo)
    add_subdirectory(demo)
endif()

if (IONIK__ENABLE_PROFILER)
    message(STATUS "Profiler feature enabled")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "-pg")
        set(CMAKE_C_FLAGS "-pg")
    else()
        message(FATAL_ERROR "Profiler feature requires GCC")
    endif()
endif()

include(GNUInstallDirs)

install(TARGETS ionik
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
