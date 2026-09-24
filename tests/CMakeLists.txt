#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   September 2026
#
# Autodoc:   yes
#==============================================================================

if(NOT TARGET vne3dgs)
    return()
endif()

#==============================================================================
# Setup Google Test
#==============================================================================
if(EXISTS "${PROJECT_SOURCE_DIR}/deps/external/googletest/CMakeLists.txt")
    message(STATUS "Using Google Test from deps/external/googletest (v1.17.0)")
    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/external/googletest ${CMAKE_BINARY_DIR}/deps/external/googletest)
    include(GoogleTest)
    set(GTEST_LIBRARIES gtest_main gmock)
    set(GTEST_INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/deps/external/googletest/googletest/include
        ${PROJECT_SOURCE_DIR}/deps/external/googletest/googlemock/include
    )
else()
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Using system Google Test")
        set(GTEST_LIBRARIES GTest::gtest_main GTest::gmock)
        set(GTEST_INCLUDE_DIRS "")
    else()
        message(STATUS "deps/external/googletest not found, using FetchContent")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.17.0
        )
        FetchContent_MakeAvailable(googletest)
        set(GTEST_LIBRARIES gtest_main gmock)
        set(GTEST_INCLUDE_DIRS ${googletest_SOURCE_DIR}/googletest/include ${googletest_SOURCE_DIR}/googlemock/include)
    endif()
endif()

#==============================================================================
# Test sources
#==============================================================================
set(TEST_SOURCES
    conic_test.cpp
    front_to_back_blender_test.cpp
    gaussian2d_test.cpp
    gaussian3d_test.cpp
    ply_reader_test.cpp
    version_test.cpp
)

#==============================================================================
# Build test executable
#==============================================================================
add_executable(vne3dgs_tests ${TEST_SOURCES})

target_include_directories(vne3dgs_tests
    PRIVATE
        ${GTEST_INCLUDE_DIRS}
        ${VNE_INCLUDE_DIR}
        ${VNE_SRC_DIR}
)

target_link_libraries(vne3dgs_tests
    PRIVATE
        ${GTEST_LIBRARIES}
        vne::gs
)

add_test(NAME vne3dgs_tests COMMAND vne3dgs_tests)
