cmake_minimum_required (VERSION 3.6)
project (VulkanRTSEngine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

# Turning on Full Optimization to help with model loading
if(MSVC)
	# spexicy all optimizations
	string(REPLACE "/Od" "/Ox" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# turn of conflicting Run Time Checks
	string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# enable inlining of everything
	string(REPLACE "/Ob0" "/Ob2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# Release already has everything except for /Ox
	string(REPLACE "/O2" "/Ox" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
elseif(CMAKE_COMPILER_IS_GNUCC)
	# not sure, have to get my hands on a linux machine
	# need to compile with -Ofast
endif()

find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED) # FindOpenGL.cmake is provided with cmake installation
find_package(GLEW REQUIRED) # FindGLEW.cmake is provided with cmake installation
find_package(GLFW REQUIRED)
find_package(GLM REQUIRED)
find_package(OpenAL REQUIRED)

file(GLOB SRC "src/*.cpp")
file(GLOB SRC_COMP "src/Components/*.cpp")
file(GLOB INC "include/*.h")
file(GLOB INC_COMP "include/Components/*.h")

add_executable(VulkanRTSEngine 
	# Our own sources...
	${SRC} ${SRC_COMP}
	# headers
	${INC} ${INC_COMP})

# just to make our life a little bit more organized, group files
source_group("Components" FILES ${SRC_COMP}	${INC_COMP})

include_directories(VulkanRTSEngine include/ include/extra 
	${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR}
	${VULKAN_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR})
target_link_libraries(VulkanRTSEngine ${GLFW_LIBRARIES} ${OPENGL_LIBRARY} ${GLEW_LIBRARY}
	${VULKAN_LIBRARY} ${OPENAL_LIBRARY})

add_custom_command(TARGET VulkanRTSEngine PRE_BUILD COMMAND ${CMAKE_COMMAND}
					-E copy_directory ${CMAKE_SOURCE_DIR}/assets
					$<TARGET_FILE_DIR:VulkanRTSEngine>/assets)

# Copy our dlls to the executable
if (MSVC)
	add_custom_command(TARGET VulkanRTSEngine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_SOURCE_DIR}/dlls $<TARGET_FILE_DIR:VulkanRTSEngine>)
endif()