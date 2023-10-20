################################################################################
# Copyright (c) 2021-2023 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2023.10.19 Initial version.
################################################################################
project(audio_spectrum)

portable_target(ADD_EXECUTABLE ${PROJECT_NAME})
portable_target(SOURCES ${PROJECT_NAME} main.cpp)

find_package(Qt5 COMPONENTS Core Gui Multimedia Qml Quick)

if (Qt5Core_VERSION AND Qt5Multimedia_VERSION AND Qt5Gui_VERSION
        AND Qt5Qml_VERSION AND Qt5Quick_VERSION)
    portable_target(DEFINITIONS ${PROJECT_NAME} PRIVATE "IONIK__QT5_GUI_ENABLED=1")
    portable_target(LINK_QT5_COMPONENTS ${PROJECT_NAME} PRIVATE Core Gui Multimedia Qml Quick)
    portable_target(SOURCES ${PROJECT_NAME} main.qrc
        WavSpectrum.cpp WavSpectrum.hpp)
endif()

if (TARGET pfs::ionik)
    portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::ionik)
elseif (TARGET pfs::ionik::static)
    portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()
