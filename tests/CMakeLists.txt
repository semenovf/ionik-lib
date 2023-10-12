################################################################################
# Copyright (c) 2023 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2023.10.12 Initial version.
################################################################################
project(ionik-TESTS CXX C)

if (IONIK__ENABLE_COVERAGE)
    message(STATUS "Coverage feature enabled")
    if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        message(FATAL_ERROR "Coverage feature requires GCC")
    endif()

    set(CMAKE_CXX_FLAGS "-g -O0 --coverage")
    set(CMAKE_C_FLAGS "-g -O0 --coverage")
endif()

if (TARGET pfs::ionik)
    set(_ionik_lib pfs::ionik)
elseif(TARGET pfs::ionik::static)
    set(_ionik_lib pfs::ionik::static)
endif()

set(TEST_NAMES file wav_explorer)

foreach (name ${TEST_NAMES})
    if (${name}_SOURCES)
        add_executable(${name} ${${name}_SOURCES} ${name}.cpp)
    else ()
        add_executable(${name} ${name}.cpp)
    endif()

    # https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#Instrumentation-Options
    #
    # --coverage
    #       This option is used to compile and link code instrumented for
    #       coverage analysis. The option is a synonym for -fprofile-arcs
    #       -ftest-coverage (when compiling) and -lgcov (when linking).
    if (IONIK__ENABLE_COVERAGE)
        target_compile_options(${name} PRIVATE -g -O0 --coverage)
        target_link_libraries(${name} PRIVATE -g -O0 --coverage)
    endif()

    target_link_libraries(${name} PRIVATE ${_ionik_lib})
    add_test(NAME ${name} COMMAND ${name})
endforeach()

if (IONIK__ENABLE_COVERAGE)
    coverage_target("'/usr/*';'*/doctest.h'")
endif()
