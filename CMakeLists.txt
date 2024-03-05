cmake_minimum_required(VERSION 3.20.0)
project(test)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(X11 REQUIRED)
include_directories(src ${Vulkan_INCLUDE_DIRS} ) # ${X11_Xxf86vm_INCLUDE_PATH} ${X11_Xrandr_INCLUDE_PATH} ${X11_Xi_INCLUDE_PATH} ${GLFW_INCLUDE_DIRS}

aux_source_directory(src SRC)
add_executable(test_vulkan main.cpp ${SRC})
target_link_libraries(test_vulkan PRIVATE ${Vulkan_LIBRARIES} dl pthread  ) # X11 Xxf86vm Xrandr Xi glfw ${X11_Xxf86vm_LIB} ${X11_Xrandr_INCLUDE_PATH} ${X11_Xi_LIB} ${GLFW3_LIBRARY}