cmake_minimum_required(VERSION 3.20.0)
project(test)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(X11 REQUIRED)
include_directories(src ${Vulkan_INCLUDE_DIRS} ) # ${X11_Xxf86vm_INCLUDE_PATH} ${X11_Xrandr_INCLUDE_PATH} ${X11_Xi_INCLUDE_PATH} ${GLFW_INCLUDE_DIRS}

set(ELT_COUNT 1024)
set(WORKGROUP_SIZE 32)
find_program(GLSLC_PROGRAM glslc REQUIRED)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shader/sum_two_vec.comp 
                         -o ${CMAKE_BINARY_DIR}/sum_two_vec.spv -DELT_COUNT=${ELT_COUNT} 
                         -DWORKGROUP_SIZE=${WORKGROUP_SIZE})

aux_source_directory(src SRC)
add_executable(test_vulkan main.cpp ${SRC})
target_link_libraries(test_vulkan PRIVATE ${Vulkan_LIBRARIES} dl pthread) # X11 Xxf86vm Xrandr Xi glfw ${X11_Xxf86vm_LIB} ${X11_Xrandr_INCLUDE_PATH} ${X11_Xi_LIB} ${GLFW3_LIBRARY}