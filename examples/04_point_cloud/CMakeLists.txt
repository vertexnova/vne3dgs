#==============================================================================
# 04_point_cloud Example: orbit camera + Gaussian centers as a point cloud PNG
#==============================================================================

add_executable(example_04_point_cloud main.cpp)

target_link_libraries(example_04_point_cloud PRIVATE
    vne::gs
    vne::logging
    vne::io::image
)

target_include_directories(example_04_point_cloud PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_04_point_cloud PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
