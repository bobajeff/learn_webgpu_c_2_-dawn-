#---------- HELLO_TRIANGLE
add_executable(hello_triangle
2_hello_triangle/hello_triangle.c
)
target_link_libraries(hello_triangle PRIVATE glfw webgpu_dawn glfw3webgpu)
