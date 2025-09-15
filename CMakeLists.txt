cmake_minimum_required(VERSION 3.10)
project(hello)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(ext/glad)      # opengl loader
add_subdirectory(ext/glfw)      # window and input manager
add_subdirectory(ext/glm)       # math
add_subdirectory(ext/stb_image) # image loader

add_executable(hello main.cpp)
target_link_libraries(hello glad glfw glm stb_image)

add_custom_command(             # resources folder
  TARGET hello POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:hello>/res
)