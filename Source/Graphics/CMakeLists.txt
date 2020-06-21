# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (Graphics LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

# TODO: provide a switch per project to download binaries only or sources
include(FetchContent)

FetchContent_Declare(
	nlohmann_json
	GIT_REPOSITORY	https://github.com/nlohmann/json
	GIT_TAG			v3.8.0
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
	"Camera.cpp" "Camera.h"
	"Descriptor.cpp" "Descriptor.h"
	"GPUResource.cpp" "GPUResource.h"
	"Graphics.cpp" "Graphics.h"
	"GraphicsTypes.h"
	"RenderContext.h"
	"RenderPass.cpp" "RenderPass.h"
	"RenderPassJob.cpp" "RenderPassJob.h"
	"UniformBlock.cpp" "UniformBlock.h"
)

set(SRC_INTERFACES "Interfaces/IModel.h"
	"Interfaces/IPipeline.h"
	"Interfaces/IShader.h"
	"Interfaces/ITexture.h"
)

set(SRC_RESOURCES "Resources/GPUModel.h"
	"Resources/GPUPipeline.h"
	"Resources/GPUShader.h"
	"Resources/GPUTexture.h"
	"Resources/Model.cpp" "Resources/Model.h"
	"Resources/Pipeline.cpp" "Resources/Pipeline.h"
	"Resources/Shader.cpp" "Resources/Shader.h"
	"Resources/Texture.cpp" "Resources/Texture.h"
)

add_library(${PROJECT_NAME} STATIC 
	${SRC} 
		${SRC_INTERFACES}
		${SRC_RESOURCES}
)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})
source_group(Interfaces FILES ${SRC_INTERFACES})
source_group(Resources FILES ${SRC_RESOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE Precomp.h)

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

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})