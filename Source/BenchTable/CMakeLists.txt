# by Daniel Prihodko
cmake_minimum_required (VERSION 3.6)
project (BenchTable LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
	googleBench
	GIT_REPOSITORY https://github.com/google/benchmark
	GIT_TAG        v1.5.1
)
FetchContent_GetProperties(googleBench)
if(NOT googleBench_POPULATED)
	FetchContent_Populate(googleBench)

	# enable our configuration
	set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_BUILD_32_BITS OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER GoogleBench)
	add_subdirectory(${googlebench_SOURCE_DIR} ${googlebench_BINARY_DIR})
endif()

set(CMAKE_FOLDER "")

file(GLOB SRC "*.cpp" "*.h")
source_group("" FILES ${SRC})

add_executable(${PROJECT_NAME} ${SRC})
#if (MSVC)
#   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
#   set_source_files_properties("Precomp.cpp" PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
#endif(MSVC)

target_link_libraries(${PROJECT_NAME} Engine benchmark::benchmark)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir)")