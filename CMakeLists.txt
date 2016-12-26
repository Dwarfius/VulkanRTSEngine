cmake_minimum_required (VERSION 3.6)
project (VulkanRTSEngine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

find_package(Vulkan REQUIRED)
find_package(GLFW REQUIRED)
find_package(GLM REQUIRED)

add_executable(VulkanRTSEngine 
# Our own sources...
src/main.cpp src/Camera.cpp src/Game.cpp src/Graphics.cpp
# headers...
include/Common.h include/Camera.h include/Game.h include/Graphics.h
# and 3rd-party stuff
src/tinyxml2.cpp include/tinyxml2.h)

include_directories(VulkanRTSEngine include/ ${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${VULKAN_INCLUDE_DIR})

target_link_libraries(VulkanRTSEngine ${GLFW_LIBRARIES} ${VULKAN_LIBRARY})

add_custom_command(TARGET VulkanRTSEngine PRE_BUILD COMMAND ${CMAKE_COMMAND}
					-E copy_directory ${CMAKE_SOURCE_DIR}/assets
					$<TARGET_FILE_DIR:VulkanRTSEngine>/assets)