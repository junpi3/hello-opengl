cmake_minimum_required(VERSION 3.10)
project(hello)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(ext/glad)      # opengl loader
add_subdirectory(ext/glfw)      # window and input manager
add_subdirectory(ext/glm)       # math
add_subdirectory(ext/miniaudio) # audio
add_subdirectory(ext/stb_image) # image loader

add_executable(hello main.cpp)
target_link_libraries(hello glad glfw glm miniaudio stb_image)

add_custom_target(copy_shaders ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/glsl ${CMAKE_BINARY_DIR}/glsl
  DEPENDS ${CMAKE_SOURCE_DIR}/glsl
  BYPRODUCTS ${CMAKE_BINARY_DIR}/glsl
)

add_custom_target(copy_resources ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res ${CMAKE_BINARY_DIR}/res
  DEPENDS ${CMAKE_SOURCE_DIR}/res
  BYPRODUCTS ${CMAKE_BINARY_DIR}/res
)

add_dependencies(hello copy_shaders copy_resources)