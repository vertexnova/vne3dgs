#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   September 2026
#
# Autodoc:   yes
#==============================================================================

# conic.h, gaussian2d.h and front_to_back_blender.h are header-only: their
# per-pixel math must inline, which an exported out-of-line definition prevents.
set(SOURCE_FILES
    vertexnova/gs/camera/camera.cpp
    vertexnova/gs/core/gaussian3d.cpp
    vertexnova/gs/core/gaussian_cloud.cpp
    vertexnova/gs/io/ply_format.cpp
    vertexnova/gs/io/ply_reader.cpp
    vertexnova/gs/io/ply_writer.cpp
    vertexnova/gs/render/cpu/point_renderer.cpp
    vertexnova/gs/render/image.cpp
    vertexnova/gs/version.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/gs/camera/camera.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/camera/conventions.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/core/conic.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/core/front_to_back_blender.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/core/gaussian2d.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/core/gaussian3d.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/core/gaussian_cloud.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/io/ply_reader.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/io/ply_writer.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/render/cpu/point_renderer.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/render/image.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/export.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/gs.h
    ${VNE_INCLUDE_DIR}/vertexnova/gs/version.h
)

# Private headers: under src/, not installed, not part of the public API.
set(PRIVATE_HEADER_FILES
    vertexnova/gs/io/ply_format.h
)

# One library per build: static or shared, always named vne3dgs (alias vne::gs).
if(VNE_GS_LIB_TYPE STREQUAL "shared")
    set(_lib_type SHARED)
else()
    set(_lib_type STATIC)
endif()

add_library(vne3dgs ${_lib_type} ${SOURCE_FILES} ${PRIVATE_HEADER_FILES} ${HEADER_FILES})

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

# C4251 ("class needs to have dll-interface to be used by clients") and C4275
# (non-dll-interface base class) fire on MSVC shared builds when a
# `__declspec(dllexport)` type holds vnemath Vec/Mat (or STL). The types are
# header-only; both DLL and consumer already share the same CRT. PUBLIC so
# tests and examples that include the exported headers inherit the suppression.
if(MSVC)
    target_compile_options(vne3dgs PUBLIC /wd4251 /wd4275)
endif()

source_group("Sources" FILES ${SOURCE_FILES} ${PRIVATE_HEADER_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})
