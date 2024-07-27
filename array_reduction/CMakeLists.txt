cmake_minimum_required(VERSION 3.20.0)
project(array_reduction)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_INIT})
endif()

find_program(GLSLC_PROGRAM glslc REQUIRED)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
message(STATUS "CMD : ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.comp -o ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${array_reduction}.spv")
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.comp
                                          -o ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${PROJECT_NAME}.spv)
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})