cmake_minimum_required(VERSION 3.20.0)
project(sum_two_array)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS} )

# aux_source_directory(chapter2 CHAPTER2)
# add_executable(c2 ${CHAPTER2})
# target_link_libraries(c2 PRIVATE ${Vulkan_LIBRARIES} )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_INIT})
endif()

find_program(GLSLC_PROGRAM glslc REQUIRED)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
message(STATUS "CMD : ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/sum_two_array.comp -o ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/two_array_sum.spv")
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/sum_two_array.comp
                                          -o ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/sum_two_array.spv)
add_executable(s2a main.cpp)
target_link_libraries(s2a PRIVATE ${Vulkan_LIBRARIES})