#---------- HELPER V3 (mostly so I don't have to figure out why cmake 
#    won't include the 3_input_geometry dir when I add helper_v2 to libraries)
add_library(helper_v3 5_3d_meshes/helper_v3.c)
target_include_directories(helper_v3 PUBLIC ${CMAKE_SOURCE_DIR}/5_3d_meshes)
target_link_libraries(helper_v3 PRIVATE webgpu_dawn)

#---------- A_SIMPLE_EXAMPLE
add_executable(a_simple_example
5_3d_meshes/a_simple_example.c
)
target_compile_definitions(a_simple_example PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/5_3d_meshes/resources"
)
target_link_libraries(a_simple_example PRIVATE glfw webgpu_dawn glfw3webgpu helper_v3)
