# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (BenchTable LANGUAGES CXX)

include(FetchContent)

SET(BENCHTABLE_GenSerialization FALSE CACHE BOOL "Should BenchTable include Serialization tests")
SET(BENCHTABLE_GenProfiling FALSE CACHE BOOL "Should BenchTable include Profiling tests")
SET(BENCHTABLE_GenPool FALSE CACHE BOOL "Should BenchTable include Pool tests")
SET(BENCHTABLE_GenSkeleton FALSE CACHE BOOL "Should BenchTable include Skeleton tests")
SET(BENCHTABLE_GenDirList FALSE CACHE BOOL "Should BenchTable include DirList tests")

FetchContent_Declare(
	googleBench
	GIT_REPOSITORY https://github.com/google/benchmark
	GIT_TAG        v1.5.5
)
FetchContent_GetProperties(googleBench)
if(NOT googlebench_POPULATED)
	FetchContent_Populate(googleBench)

	# enable our configuration
	set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_BUILD_32_BITS OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER Extern/GoogleBench)
	add_subdirectory(${googlebench_SOURCE_DIR} ${googlebench_BINARY_DIR})
endif()

set(CMAKE_FOLDER "")

file(GLOB SRC LIST_DIRECTORIES false *)

if(BENCHTABLE_GenSerialization)
	file(GLOB_RECURSE SRC_EXTRA Serialization/*)
	list(APPEND SRC ${SRC_EXTRA})
endif()

if(BENCHTABLE_GenProfiling)
	set(SRC_EXTRA "Profiling/ChronoProfiler.cpp"	
		"Profiling/CommonTypes.h"
	)
	if(WIN32)
		list(APPEND SRC_EXTRA "Profiling/QPCProfiler.cpp")
	endif()
	list(APPEND SRC ${SRC_EXTRA})
endif()

if(BENCHTABLE_GenPool)
	file(GLOB_RECURSE SRC_EXTRA Pool/*)
	list(APPEND SRC ${SRC_EXTRA})
endif()

if(BENCHTABLE_GenSkeleton)
	file(GLOB_RECURSE SRC_EXTRA Skeleton/*)
	list(APPEND SRC ${SRC_EXTRA})
endif()

if(BENCHTABLE_GenDirList)
	file(GLOB_RECURSE SRC_EXTRA DirList/*)
	list(APPEND SRC ${SRC_EXTRA})
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC})
add_executable(${PROJECT_NAME} ${SRC})

target_precompile_headers(${PROJECT_NAME} PRIVATE Precomp.h)

target_include_directories(${PROJECT_NAME}
	PRIVATE 
		. 
		Extra
)

target_link_libraries(${PROJECT_NAME} Engine benchmark::benchmark)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir)")