#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vne3dgs)
    return()
endif()

# 00 - Hello: minimal usage of vne::gs (getVersion)
add_subdirectory(00_hello_gs)

# Each learning task adds its example here, e.g.:
# add_subdirectory(01_gaussians_2d)   # Task 01
