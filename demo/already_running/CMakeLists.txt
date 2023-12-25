################################################################################
# Copyright (c) 2021-2023 Vladislav Trifochkin
#
# This file is part of `ionik-lib`.
#
# Changelog:
#      2023.12.24 Initial version.
################################################################################
project(already_running)

portable_target(ADD_EXECUTABLE ${PROJECT_NAME})
portable_target(SOURCES ${PROJECT_NAME} main.cpp)

if (TARGET pfs::ionik)
    portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::ionik)
elseif (TARGET pfs::ionik::static)
    portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::ionik::static)
endif()
