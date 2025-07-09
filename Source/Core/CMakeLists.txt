# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (Core LANGUAGES CXX)

# TODO: provide a switch per project to download binaries only or sources
include(FetchContent)

FetchContent_Declare(
	tbb
	GIT_REPOSITORY	https://github.com/oneapi-src/oneTBB
	GIT_TAG			v2022.0.0
	SOURCE_SUBDIR 	DoesNotExist # prevents add_subdir, so that we can manually configure
)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm
	GIT_TAG			1.0.1
	SOURCE_SUBDIR 	DoesNotExist # prevents add_subdir, so that we can manually configure
)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw
	GIT_TAG 3.4
	SOURCE_SUBDIR 	DoesNotExist # prevents add_subdir, so that we can manually configure
)

FetchContent_Declare(
	nlohmann_json
	GIT_REPOSITORY	https://github.com/nlohmann/json
	GIT_TAG			v3.11.3
	SOURCE_SUBDIR 	DoesNotExist # prevents add_subdir, so that we can manually configure
)

# download the repos
FetchContent_MakeAvailable(tbb glm glfw nlohmann_json)

# Now we can manually add them
# TBB
set(TBB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL "" FORCE)
set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL "" FORCE)

set(TBB_TEST OFF CACHE BOOL "" FORCE)
set(TBB_BENCH OFF CACHE BOOL "" FORCE)

set(CMAKE_FOLDER Extern/ThreadBuildingBlocks)
add_subdirectory(${tbb_SOURCE_DIR} ${tbb_BINARY_DIR} EXCLUDE_FROM_ALL)

# TBB on Win32 implicitly links libraries, which can go
# against CMake's custom targets like RelWithDebInfo
if (MSVC)
	target_compile_definitions(tbb PUBLIC __TBB_NO_IMPLICIT_LINKAGE=1)
endif()

set_target_properties(tbb PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})

if(VENGINE_ENABLE_UNITYBUILDS)
	# Sadly, these 2 files aren't friendly in unity builds, so gotta exclude them
	set(TBB_UNITY_BADDIES 
		${tbb_SOURCE_DIR}/src/tbb/parallel_pipeline.cpp
		${tbb_SOURCE_DIR}/src/tbb/itt_notify.cpp
	)
	set_source_files_properties(${TBB_UNITY_BADDIES} 
		TARGET_DIRECTORY tbb
		PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
	)
endif()

# GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
set(CMAKE_FOLDER Extern/GLM)
add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})

# GLFW
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR} EXCLUDE_FROM_ALL)

# GLFW sets it's own CMAKE_FOLDER property via set_target_properties(... FOLDER ...)
# which ignores global CMAKE_FOLDER. since we want to control how we organize 
# dependencies we'll override it
set_target_properties(glfw PROPERTIES
	FOLDER "Extern/GLFW3"
)
set_target_properties(update_mappings PROPERTIES
	FOLDER "Extern/GLFW3"
)

set_target_properties(glfw PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})

# NLohmannJson
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Coverage OFF CACHE BOOL "" FORCE)
set(JSON_MultipleHeaders ON CACHE BOOL "" FORCE)
set(JSON_NoExceptions ON CACHE BOOL "" FORCE)
set(JSON_Sanitizer OFF CACHE BOOL "" FORCE)
set(JSON_Valgrind OFF CACHE BOOL "" FORCE)

# Removes extra generated projects
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR})

# back to ours

set(CMAKE_FOLDER "")

file(GLOB_RECURSE SRC *)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC})
add_library(${PROJECT_NAME} STATIC ${SRC})

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

option(VENGINE_USE_ASAN "Enable AddressSanitizer" OFF)
if(VENGINE_USE_ASAN)
	target_compile_options(${PROJECT_NAME} PUBLIC "/fsanitize=address")
endif()

target_include_directories(${PROJECT_NAME}
	PRIVATE 
		. 
		Extra
	INTERFACE
		..
)

if (MSVC)
   set(EXTRA_LIBS "IPHLPAPI.lib") # for UID class
endif(MSVC)

target_link_libraries(${PROJECT_NAME} 
	${EXTRA_LIBS}
	glm
	nlohmann_json::nlohmann_json
	tbb
	glfw
)

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})