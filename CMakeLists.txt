# by Daniel Prihodko
cmake_minimum_required (VERSION 3.12)
project(VEngine LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Redirecting build output to /bin, so that we can easily access assets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_LIST_DIR}/bin)

# Whether to build with audio support or not.
# Requires installed OpenAL sdk
option(VENGINE_BUILD_WITH_AUDIO "Build with audio support" OFF)

# Whether to build with a secondary rendering backend or not
# Requires Vulkan sdk
option(VENGINE_BUILD_WITH_VULKAN "[Unsupported] Build with Vulkan support" OFF)
if(VENGINE_BUILD_WITH_VULKAN)
	set(VENGINE_BUILD_WITH_VULKAN OFF)
	message("Currently Vulkan backend is in the works and won't compile, so the switch has been turned off.")
endif()

# Whether to generate BenchTable projects or not
# BenchTable is a benchmark playground, which doesn't need to always be there
# since it slows down BuildAll build process
option(VENGINE_GENERATE_BENCHTABLE "Generate BenchTable project - turn off to speed up BuildAll" OFF)

# Enables UNity builds of our projects
option(VENGINE_ENABLE_UNITYBUILDS "Enable Unity builds for VEngine projects" ON)

add_subdirectory(Source/Extern)
add_subdirectory(Source/Core)
add_subdirectory(Source/Physics)
add_subdirectory(Source/Graphics)
add_subdirectory(Source/Engine)
add_subdirectory(Source/WorldEditor)
add_subdirectory(Source/DungeonEditor)
add_subdirectory(Source/StressTest)

if(VENGINE_GENERATE_BENCHTABLE)
	add_subdirectory(Source/BenchTable)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DungeonEditor)