# by Daniel Prihodko
cmake_minimum_required (VERSION 3.6)
project(VEngine LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Redirecting build output to /bin, so that we can easily access assets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_LIST_DIR}/bin)

# Whether to build with audio support or not.
# Requires installed OpenAL sdk
option(VENGINE_BUILD_WITH_AUDIO OFF)

# Whether to build with a secondary rendering backend or not
# Requires Vulkan sdk
option(VENGINE_BUILD_WITH_VULKAN OFF)
if(VENGINE_BUILD_WITH_VULKAN)
	set(VENGINE_BUILD_WITH_VULKAN OFF)
	message("Currently Vulkan backend is in the works and won't compile, so the switch has been turned off.")
endif()

add_subdirectory(Source/Core)
add_subdirectory(Source/Physics)
add_subdirectory(Source/Graphics)
add_subdirectory(Source/Engine)
add_subdirectory(Source/Executable)
