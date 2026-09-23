#==============================================================================
# 03_ply_stats Example: load a 3DGS PLY and print cloud statistics
#==============================================================================

add_executable(example_03_ply_stats main.cpp)

target_link_libraries(example_03_ply_stats PRIVATE
    vne::gs
    vne::logging
)

target_include_directories(example_03_ply_stats PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_03_ply_stats PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
