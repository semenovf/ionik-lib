################################################################################
# Copyright (c) 2023-2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#       2023.10.12 Initial version.
#       2024.11.23 Removed `portable_target` dependency.
################################################################################
project(ionik-TESTS CXX C)

# Copy test files to build folder
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/au DESTINATION data)

set(TEST_NAMES file wav_explorer)

foreach (name ${TEST_NAMES})
    if (${name}_SOURCES)
        add_executable(${name} ${${name}_SOURCES} ${name}.cpp)
    else ()
        add_executable(${name} ${name}.cpp)
    endif()

    target_link_libraries(${name} PRIVATE pfs::ionik)
    add_test(NAME ${name} COMMAND ${name})
endforeach()

find_package(Qt5 COMPONENTS Core Multimedia)

if (Qt5Core_VERSION AND Qt5Multimedia_VERSION)
    target_compile_definitions(wav_explorer PRIVATE "IONIK__QT5_MULTIMEDIA_ENABLED=1")
    target_link_libraries(wav_explorer PRIVATE Qt5::Multimedia Qt5::Core)
endif()
