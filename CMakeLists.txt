cmake_minimum_required (VERSION 3.6)
project (VulkanRTSEngine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

find_package(Vulkan REQUIRED)
find_package(GLFW REQUIRED)
find_package(GLM REQUIRED)

add_executable(VulkanRTSEngine src/main.cpp include/Common.h)

include_directories(VulkanRTSEngine include/ ${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${VULKAN_INCLUDE_DIR})

target_link_libraries(VulkanRTSEngine ${GLFW_LIBRARIES} ${VULKAN_LIBRARY})

add_custom_command(TARGET VulkanRTSEngine PRE_BUILD COMMAND ${CMAKE_COMMAND}
					-E copy_directory ${CMAKE_SOURCE_DIR}/assets
					$<TARGET_FILE_DIR:VulkanRTSEngine>/assets)