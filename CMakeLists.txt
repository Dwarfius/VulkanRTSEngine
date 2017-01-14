cmake_minimum_required (VERSION 3.6)
project (VulkanRTSEngine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

# Avoiding bloating by focusing either on Vulkan or GL
option(UseVulkan "Use Vulkan" OFF)

if(UseVulkan)
	find_package(Vulkan REQUIRED)
	set(GraphicsCpp src/GraphicsVK.cpp)
	set(GraphicsH include/GraphicsVK.h)
else()
	find_package(OpenGL REQUIRED)
	find_package(GLEW REQUIRED) # FindGLEW.cmake is provided with cmake installation
	set(GraphicsCpp src/GraphicsGL.cpp)
	set(GraphicsH include/GraphicsGL.h)
endif()

find_package(GLFW REQUIRED)
find_package(GLM REQUIRED)

add_executable(VulkanRTSEngine 
	# Our own sources...
	src/main.cpp src/Camera.cpp src/Game.cpp ${GraphicsCpp}
	# headers...
	include/Common.h include/Camera.h include/Game.h include/Graphics.h ${GraphicsH}
	# and 3rd-party stuff
	src/tinyxml2.cpp include/tinyxml2.h)

if(UseVulkan)
	add_definitions(-DGRAPHICS_VK)
	include_directories(VulkanRTSEngine include/ ${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${VULKAN_INCLUDE_DIR})
	target_link_libraries(VulkanRTSEngine ${GLFW_LIBRARIES} ${VULKAN_LIBRARY})
else()
	add_definitions(-DGRAPHICS_GL)
	include_directories(VulkanRTSEngine include/ ${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
	target_link_libraries(VulkanRTSEngine ${GLFW_LIBRARIES} ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
endif()

add_custom_command(TARGET VulkanRTSEngine PRE_BUILD COMMAND ${CMAKE_COMMAND}
					-E copy_directory ${CMAKE_SOURCE_DIR}/assets
					$<TARGET_FILE_DIR:VulkanRTSEngine>/assets)

# Copy our dlls to the executable
if (MSVC)
	add_custom_command(TARGET VulkanRTSEngine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_SOURCE_DIR}/dlls $<TARGET_FILE_DIR:VulkanRTSEngine>)
endif()