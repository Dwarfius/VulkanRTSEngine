# by Daniel Prihodko
cmake_minimum_required (VERSION 3.6)
project(VEngine LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

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
