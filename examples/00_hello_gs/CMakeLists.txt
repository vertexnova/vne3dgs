#==============================================================================
# 00_hello_gs Example: minimal vne3dgs usage (prints the library version)
#==============================================================================

add_executable(example_00_hello_gs main.cpp)

target_link_libraries(example_00_hello_gs PRIVATE vne::gs vne::logging)

target_include_directories(example_00_hello_gs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_00_hello_gs PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
