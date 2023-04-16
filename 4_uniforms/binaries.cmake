#---------- A_FIRST_UNIFORM
add_executable(a_first_uniform
4_uniforms/a_first_uniform.c
)
target_compile_definitions(a_first_uniform PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/4_uniforms/resources"
)
target_link_libraries(a_first_uniform PRIVATE glfw webgpu_dawn glfw3webgpu helper_v1)
