# by Daniel Prihodko
cmake_minimum_required (VERSION 3.11)
project (Core LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

# TODO: provide a switch per project to download binaries only or sources
include(FetchContent)

FetchContent_Declare(
	tbb
	GIT_REPOSITORY	https://github.com/01org/tbb
	GIT_TAG			2019_U3	
)

FetchContent_GetProperties(tbb)
if(NOT tbb_POPULATED)
	FetchContent_Populate(tbb)

	# Intel doesn't provide a proper CMakeLists, only TBBBuild.cmake script
	# which requires us to have gmake on win32... to avoid requiring it
	# we provide our own CMakeLists.txt
	file(COPY ../../cmake/TBB-CMakeLists.txt
		DESTINATION ${tbb_SOURCE_DIR}
	)
	file(RENAME ${tbb_SOURCE_DIR}/TBB-CMakeLists.txt
		${tbb_SOURCE_DIR}/CMakeLists.txt)

	set(TBB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(TBB_BUILD_STATIC ON CACHE BOOL "" FORCE)
	set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL "" FORCE)
	set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL "" FORCE)
	set(TBB_BUILD_TESTS OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER ThreadBuildingBlocks)
	add_subdirectory(${tbb_SOURCE_DIR} ${tbb_BINARY_DIR})
endif()

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

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm
	GIT_TAG			0.9.9.3
)

FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
	FetchContent_Populate(glm)
	set(CMAKE_FOLDER GLM)
	add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})

	# in CMake 3.13 has an issue where it skips custom_targets for CMAKE_FOLDER var
	# https://gitlab.kitware.com/cmake/cmake/issues/18372
	# So have to do it manually
	set_property(TARGET uninstall PROPERTY FOLDER GLM)
endif()

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw
	GIT_TAG 3.2.1
)

FetchContent_GetProperties(glfw)
if(NOT glfw_POPULATED)
	FetchContent_Populate(glfw)

	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

	add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
endif()

set(CMAKE_FOLDER "")

set(SRC "Precomp.cpp" "Precomp.h"
	"Camera.cpp" "Camera.h"
	"CRC32.h"
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

set(SRC_DEBUG "Debug/Assert.cpp" "Debug/Assert.h" "Debug/Assert.inl" 
	"Debug/DebugDrawer.cpp" "Debug/DebugDrawer.h"
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
		__STDC_WANT_LIB_EXT1__=1 # to get safe format functions from stdio
)

target_include_directories(${PROJECT_NAME}
	PRIVATE 
		. 
		Extra
	INTERFACE
		..
)

target_link_libraries(${PROJECT_NAME} 
	${EXTRA_LIBS}
	glm
	nlohmann_json::nlohmann_json
	tbb_static # TODO: will need to generalize it
	glfw
)