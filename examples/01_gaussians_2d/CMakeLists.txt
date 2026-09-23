#==============================================================================
# 01_gaussians_2d Example: five 2D Gaussians, alpha-blended to PNG via vneio
#==============================================================================

add_executable(example_01_gaussians_2d main.cpp)

target_link_libraries(example_01_gaussians_2d PRIVATE
    vne::gs
    vne::logging
    vne::io::image
)

target_include_directories(example_01_gaussians_2d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_01_gaussians_2d PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
