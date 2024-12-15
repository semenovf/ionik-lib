################################################################################
# Copyright (c) 2024 Vladislav Trifochkin
#
# This file is part of `ionic-lib`.
#
# Changelog:
#       2024.12.15 Initial version.
################################################################################
project(win32_service)

if (NOT MSVC)
    return()
endif()

set(MC_COMMAND mc)
set(RC_COMMAND rc)
set(LINK_COMMAND link)

add_custom_target(${PROJECT_NAME}_messages
    COMMAND ${MC_COMMAND} -U ${CMAKE_CURRENT_LIST_DIR}/messages.mc
    COMMAND ${RC_COMMAND} -nologo -r messages.rc
    COMMAND ${LINK_COMMAND} -nologo -dll -noentry /MACHINE:${IONIK__SERVICE_PLATFORM} -out:messages.dll messages.res
    SOURCES ${CMAKE_CURRENT_LIST_DIR}/messages.mc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS 
        ${CMAKE_CURRENT_BINARY_DIR}/messages.h
        ${CMAKE_CURRENT_BINARY_DIR}/messages.dll
    VERBATIM)

add_executable(ionik_svc service.cpp)
add_executable(ionik_svc_config service_config.cpp)
add_executable(ionik_svc_control service_control.cpp)

# Include directory for generated files (e.g. from Message Compiler)
target_include_directories(ionik_svc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

foreach (_target ionik_svc ionik_svc_config ionik_svc_control)
    target_compile_definitions(${_target} PRIVATE UNICODE _UNICODE)
    target_compile_options(${_target} PRIVATE "/utf-8")
    target_link_libraries(${_target} PRIVATE pfs::ionik)
    add_dependencies(${_target} ${PROJECT_NAME}_messages)
endforeach()
