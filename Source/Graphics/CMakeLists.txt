# by Daniel Prihodko
cmake_minimum_required (VERSION 3.11)
project (Graphics LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

# TODO: provide a switch per project to download binaries only or sources
include(FetchContent)

FetchContent_Declare(
	nlohmann_json
	GIT_REPOSITORY	https://github.com/nlohmann/json
	GIT_TAG			v3.4.0
)

FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
	FetchContent_Populate(nlohmann_json)

	# Since we grab a particular version, we know that it
	# is stable, so we can get rid of tests and other bits
	set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
	set(JSON_Coverage OFF CACHE BOOL "" FORCE)
	set(JSON_MultipleHeaders ON CACHE BOOL "" FORCE)
	set(JSON_NoExceptions ON CACHE BOOL "" FORCE)
	set(JSON_Sanitizer OFF CACHE BOOL "" FORCE)
	set(JSON_Valgrind OFF CACHE BOOL "" FORCE)

	# Removes extra generated projects
	set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
	add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR})
endif()

set(SRC "Precomp.cpp" "Precomp.h"
	"AssetTracker.cpp" "AssetTracker.h"
	"Camera.cpp" "Camera.h"
	"Descriptor.cpp" "Descriptor.h"
	"Graphics.cpp" "Graphics.h"
	"GraphicsTypes.h"
	"Model.cpp" "Model.h"
	"Pipeline.cpp" "Pipeline.h"
	"RenderContext.h"
	"RenderPass.cpp" "RenderPass.h"
	"RenderPassJob.cpp" "RenderPassJob.h"
	"Resource.cpp" "Resource.h"
	"Shader.cpp" "Shader.h"
	"Texture.cpp" "Texture.h"
	"UniformBlock.cpp" "UniformBlock.h"
)

add_library(${PROJECT_NAME} STATIC ${SRC} ${SRC_THREADING} ${SRC_DEBUG} ${SRC_GRAPHICS})
if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
   set_source_files_properties("Precomp.cpp" PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
endif(MSVC)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})

target_include_directories(${PROJECT_NAME}
	PRIVATE 
		. 
		Extra
	INTERFACE
		..
)

target_link_libraries(${PROJECT_NAME} 
	Core
	nlohmann_json::nlohmann_json
)