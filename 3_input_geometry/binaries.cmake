add_library(helper_v1 3_input_geometry/helper.c)
target_include_directories(helper_v1 PUBLIC ${CMAKE_SOURCE_DIR}/3_input_geometry)
target_link_libraries(helper_v1 PRIVATE webgpu_dawn)

#---------- VERTEX_ATTRIBUTE
add_executable(vertex_attribute
3_input_geometry/vertex_attribute.c
)
target_link_libraries(vertex_attribute PRIVATE glfw webgpu_dawn glfw3webgpu helper_v1)

#---------- MULTIPLE ATTRIBUTES (A)
add_executable(multiple_attributes_a
3_input_geometry/multiple_attributes_a.c
)
target_link_libraries(multiple_attributes_a PRIVATE glfw webgpu_dawn glfw3webgpu helper_v1)

#---------- MULTIPLE ATTRIBUTES (B)
add_executable(multiple_attributes_b
3_input_geometry/multiple_attributes_b.c
)
target_link_libraries(multiple_attributes_b PRIVATE glfw webgpu_dawn glfw3webgpu helper_v1)

#---------- INDEX_BUFFER
add_executable(index_buffer
3_input_geometry/index_buffer.c
)
target_link_libraries(index_buffer PRIVATE glfw webgpu_dawn glfw3webgpu helper_v1)

#---------- LOADING_FROM_FILE
add_executable(loading_from_file
3_input_geometry/loading_from_file.c
)
target_compile_definitions(loading_from_file PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/3_input_geometry/resources"
)
target_link_libraries(loading_from_file PRIVATE glfw webgpu_dawn glfw3webgpu helper_v1)

#---------- LOADING_FROM_FILE EXPERIMENT
add_executable(loading_from_file_exp
3_input_geometry/loading_from_file_exp.c
)
target_compile_definitions(loading_from_file_exp PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/3_input_geometry/resources"
)
target_link_libraries(loading_from_file_exp PRIVATE glfw webgpu_dawn glfw3webgpu helper_v1)
