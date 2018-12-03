# by Daniel Prihodko
cmake_minimum_required (VERSION 3.11)
project (Core LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

# TODO: provide a switch per project to download binaries only or sources
include(FetchContent)

find_package(GLFW REQUIRED)
find_package(TBB REQUIRED) # https://github.com/01org/tbb/tree/tbb_2018/cmake

FetchContent_Declare(
	nlohmann_json
	GIT_REPOSITORY https://github.com/nlohmann/json
	GIT_TAG        v3.4.0
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

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm
	GIT_TAG			0.9.9.3
)

FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
	FetchContent_Populate(glm)
	add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})
endif()

set(SRC "Precomp.cpp" "Precomp.h"
	"Camera.cpp" "Camera.h"
	"Transform.cpp" "Transform.h"
	"UID.cpp" "UID.h"
	"RefCounted.cpp" "RefCounted.h"
	"RWBuffer.h"
	"Utils.cpp" "Utils.h"
	"Vertex.h"
) 

set(SRC_THREADING "Threading/AssertMutex.cpp" "Threading/AssertMutex.h"
	"Threading/AssertRWMutex.cpp" "Threading/AssertRWMutex.h"
)

set(SRC_GRAPHICS "Graphics/AssetTracker.cpp" "Graphics/AssetTracker.h"
	"Graphics/Descriptor.cpp" "Graphics/Descriptor.h"
	"Graphics/Graphics.cpp" "Graphics/Graphics.h"
	"Graphics/GraphicsTypes.h"
	"Graphics/Model.cpp" "Graphics/Model.h"
	"Graphics/Pipeline.cpp" "Graphics/Pipeline.h"
	"Graphics/Resource.cpp" "Graphics/Resource.h"
	"Graphics/Shader.cpp" "Graphics/Shader.h"
	"Graphics/Texture.cpp" "Graphics/Texture.h"
	"Graphics/UniformBlock.cpp" "Graphics/UniformBlock.h"
)

set(SRC_DEBUG "Debug/Assert.cpp" "Debug/Assert.h"
)

add_library(${PROJECT_NAME} STATIC ${SRC} ${SRC_THREADING} ${SRC_DEBUG} ${SRC_GRAPHICS})
if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
   set_source_files_properties("Precomp.cpp" PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
   set(EXTRA_LIBS "IPHLPAPI.lib") # for UID class
endif(MSVC)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})
source_group(Threading FILES ${SRC_THREADING})
source_group(Debug FILES ${SRC_DEBUG})
source_group(Graphics FILES ${SRC_GRAPHICS})

target_compile_definitions(${PROJECT_NAME} 
	PUBLIC 
		NOMINMAX # to prevent tbb to carry over min/max macros from win headers
		GLM_FORCE_DEPTH_ZERO_TO_ONE
		GLM_ENABLE_EXPERIMENTAL # we use transform and hash implementations
		ASSERT_MUTEX # enables utility mutex that asserts on concurrent use
		# ENABLE_ASSERTS # uncomment this to force asserts
)

target_include_directories(${PROJECT_NAME}
	PUBLIC 
		. 
		${GLFW_INCLUDE_DIR}
	PRIVATE 
		Extra
		# ${GLFW_INCLUDE_DIR} TODO: look into hiding it Core
)

target_link_libraries(${PROJECT_NAME} 
	${GLFW_LIBRARIES} 
	${TBB_IMPORTED_TARGETS} 
	${EXTRA_LIBS}
	glm
	nlohmann_json::nlohmann_json
)