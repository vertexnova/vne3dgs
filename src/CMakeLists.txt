#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   September 2026
#
# Autodoc:   yes
#==============================================================================

set(SOURCE_FILES
    vertexnova/gs/version.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/gs/export.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/gs.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/version.h
)

# One library per build: static or shared, always named vne3dgs (alias vne::gs).
if(VNE_GS_LIB_TYPE STREQUAL "shared")
    set(_lib_type SHARED)
else()
    set(_lib_type STATIC)
endif()

add_library(vne3dgs ${_lib_type} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(vne3dgs
    PUBLIC
        $<BUILD_INTERFACE:${VNE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${VNE_SRC_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(vne3dgs
    PUBLIC
        vne::gs::Warnings
        vne::gs::BuildSettings
)

if(TARGET vne::common)
    target_link_libraries(vne3dgs PUBLIC vne::common)
endif()
if(TARGET vne::logging)
    target_link_libraries(vne3dgs PUBLIC vne::logging)
endif()
if(TARGET vne::math)
    target_link_libraries(vne3dgs PUBLIC vne::math)
endif()

target_compile_features(vne3dgs PUBLIC cxx_std_20)

# Set export/import defines for shared builds. The header uses them only on Windows
# (VNE_GS_API becomes __declspec(dllexport)/dllimport); on other platforms they are no-ops.
if(_lib_type STREQUAL "SHARED")
    target_compile_definitions(vne3dgs
        PRIVATE VNE_GS_BUILDING_DLL
        INTERFACE VNE_GS_DLL
    )
endif()

add_library(vne::gs ALIAS vne3dgs)

source_group("Sources" FILES ${SOURCE_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})
