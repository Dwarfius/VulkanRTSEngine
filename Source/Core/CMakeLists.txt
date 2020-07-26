# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
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
		${tbb_SOURCE_DIR}/CMakeLists.txt
	)

	set(TBB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(TBB_BUILD_STATIC ON CACHE BOOL "" FORCE)
	set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL "" FORCE)
	set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL "" FORCE)
	set(TBB_BUILD_TESTS OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER ThreadBuildingBlocks)
	add_subdirectory(${tbb_SOURCE_DIR} ${tbb_BINARY_DIR})
endif()

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm
	GIT_TAG			0.9.9.7
)

FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
	FetchContent_Populate(glm)

	# disable unit tests
	# TODO: play around with simd toggles
	set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER GLM)
	add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})
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

set(CMAKE_FOLDER "")

set(SRC "Precomp.h"
	"CRC32.h"
	"File.cpp" "File.h"
	"LazyVector.h"
	"Pool.h"
	"Transform.cpp" "Transform.h"
	"Profiler.cpp" "Profiler.h"
	"RefCounted.cpp" "RefCounted.h"
	"RWBuffer.h"
	"StaticString.h"
	"UID.cpp" "UID.h"
	"Utils.cpp" "Utils.h"
	"Vertex.h"
)

set(SRC_ALGOS "Algos/DiamondSquareAlgo.h"
)

set(SRC_THREADING "Threading/AssertMutex.cpp" "Threading/AssertMutex.h"
	"Threading/AssertRWMutex.cpp" "Threading/AssertRWMutex.h"
)

set(SRC_DEBUG "Debug/Assert.cpp" "Debug/Assert.h" "Debug/Assert.inl" 
	"Debug/DebugDrawer.cpp" "Debug/DebugDrawer.h"
)

set(SRC_RESOURCES "Resources/AssetTracker.cpp" "Resources/AssetTracker.h"
	"Resources/JsonSerializer.cpp" "Resources/JsonSerializer.h"
	"Resources/Resource.cpp" "Resources/Resource.h"
	"Resources/Serializer.cpp" "Resources/Serializer.h"
)

add_library(${PROJECT_NAME} STATIC 
	${SRC} ${SRC_THREADING} ${SRC_DEBUG} ${SRC_ALGOS} ${SRC_RESOURCES})
if (MSVC)
   set(EXTRA_LIBS "IPHLPAPI.lib") # for UID class
endif(MSVC)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})
source_group(Algos FILES ${SRC_ALGOS})
source_group(Threading FILES ${SRC_THREADING})
source_group(Debug FILES ${SRC_DEBUG})
source_group(Resources FILES ${SRC_RESOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE Precomp.h)

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

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})