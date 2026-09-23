#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vne3dgs)
    return()
endif()

# 00 - Hello: minimal usage of vne::gs (getVersion)
add_subdirectory(00_hello_gs)

# 01 - 2D Gaussians: evaluate, alpha-blend, write PNG via vneio
if(TARGET vne::io::image)
    add_subdirectory(01_gaussians_2d)
else()
    message(STATUS "Skipping example_01_gaussians_2d (vne::io::image not found)")
endif()
