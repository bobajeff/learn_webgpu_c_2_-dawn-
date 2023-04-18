#---------- HELPER V2
add_library(helper_v2 4_uniforms/helper_v2.c)
target_include_directories(helper_v2 PUBLIC ${CMAKE_SOURCE_DIR}/3_input_geometry)
target_link_libraries(helper_v2 PRIVATE webgpu_dawn)

#---------- A_FIRST_UNIFORM
add_executable(a_first_uniform
4_uniforms/a_first_uniform.c
)
target_compile_definitions(a_first_uniform PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/4_uniforms/resources"
)
target_link_libraries(a_first_uniform PRIVATE glfw webgpu_dawn glfw3webgpu helper_v2)

#---------- MORE_UNIFORMS
add_executable(more_uniforms
4_uniforms/more_uniforms.c
)
target_compile_definitions(more_uniforms PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/4_uniforms/resources"
)
target_link_libraries(more_uniforms PRIVATE glfw webgpu_dawn glfw3webgpu helper_v2)

#---------- DYNAMIC_UNIFORMS
add_executable(dynamic_uniforms
4_uniforms/dynamic_uniforms.c
)
target_compile_definitions(dynamic_uniforms PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/4_uniforms/resources"
)
target_link_libraries(dynamic_uniforms PRIVATE glfw webgpu_dawn glfw3webgpu helper_v2)
