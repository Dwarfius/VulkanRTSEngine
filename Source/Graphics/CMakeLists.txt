# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (Graphics LANGUAGES CXX)

file(GLOB_RECURSE SRC *)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC})
add_library(${PROJECT_NAME} STATIC ${SRC})

target_precompile_headers(${PROJECT_NAME} PRIVATE Precomp.h)

target_include_directories(${PROJECT_NAME}
	PRIVATE 
		. 
	INTERFACE
		..
)

target_link_libraries(${PROJECT_NAME} 
	PUBLIC
		Core
	PRIVATE
		STB_Image
)

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})