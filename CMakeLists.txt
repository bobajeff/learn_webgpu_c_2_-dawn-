cmake_minimum_required(VERSION 3.0...3.25)
project(
	LearnWebGPU # name of the project, which will also be the name of the visual studio solution if you use it
	VERSION 0.1.0 # any version number
	LANGUAGES CXX C # programming languages used by the project
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(dawn EXCLUDE_FROM_ALL)
add_subdirectory(cglm)
# set(DAWN_ALWAYS_ASSERT ON)
add_subdirectory(glfw3webgpu)
# add_executable(App main.cpp)

# target_link_libraries(App PRIVATE
# 	webgpu_dawn
# 	glfw
# 	glfw3webgpu
# )

# target_compile_options(App PRIVATE -Wall -Wextra -pedantic)


include(1_getting_started/binaries.cmake)
include(2_hello_triangle/binaries.cmake)
include(3_input_geometry/binaries.cmake)
include(4_uniforms/binaries.cmake)
include(5_3d_meshes/binaries.cmake)

