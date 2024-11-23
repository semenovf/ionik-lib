################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#       2023.10.19 Initial version.
#       2024.11.23 Removed `portable_target` dependency.
################################################################################
project(audio_spectrum)

add_executable(${PROJECT_NAME} main.cpp)

find_package(Qt5 COMPONENTS Core Gui Multimedia Qml Quick)
target_link_libraries(${PROJECT_NAME} PRIVATE pfs::ionik)

if (Qt5Core_VERSION AND Qt5Multimedia_VERSION AND Qt5Gui_VERSION
        AND Qt5Qml_VERSION AND Qt5Quick_VERSION)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "IONIK__QT5_GUI_ENABLED=1")
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Gui Qt5::Multimedia Qt5::Qml Qt5::Quick)
    target_sources(${PROJECT_NAME} PRIVATE main.qrc WavSpectrum.cpp WavSpectrum.hpp)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        AUTOMOC ON
        AUTORCC ON)
endif()
