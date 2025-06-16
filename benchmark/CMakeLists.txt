cmake_minimum_required(VERSION 3.0)

project(benchmark)

# 检测系统字节序(大端/小端)
include (TestBigEndian)
TEST_BIG_ENDIAN(is_big_endian)

# 编译shader成hpp
add_custom_target(compile_shaders ALL)
add_custom_command(
    TARGET compile_shaders PRE_BUILD
    COMMAND ${CMAKE_COMMAND} 
    ARGS "-DINPUT_SHADER_DIR=${CMAKE_CURRENT_LIST_DIR}/shaders"
        "-DOUTPUT_HEADER_FILE=${CMAKE_CURRENT_LIST_DIR}/shaders.hpp"
        "-DHEADER_NAMESPACE=shader"
        "-DIS_BIG_ENDIAN=${is_big_endian}"
        "-DVARIABLE_NAME=comp_spv"
        "-P"
        "${CMAKE_CURRENT_LIST_DIR}/cmake/vulkan_shader_compile.cmake"
)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} main.cpp benchmark.h benchmark.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})
add_dependencies(${PROJECT_NAME} compile_shaders)
