# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (Engine LANGUAGES CXX)

include(FetchContent)

if(VENGINE_BUILD_WITH_VULKAN)
	find_package(Vulkan REQUIRED)
endif() 

find_package(OpenGL REQUIRED)

# we're going to grab a fork of glew which has proper CMakeLists and headers generated
FetchContent_Declare(
	glew
	GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
	GIT_TAG        glew-cmake-2.2.0
)

FetchContent_GetProperties(glew)
if(NOT glew_POPULATED)
	FetchContent_Populate(glew)

	# enable our configuration
	set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_SINGLE_CONTEXT ON CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_MULTI_CONTEXT OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER Extern/GLEW)
	add_subdirectory(${glew_SOURCE_DIR} ${glew_BINARY_DIR})
endif()

if(VENGINE_BUILD_WITH_AUDIO)
	# freealut is old, last commit was June 2014
	FetchContent_Declare(
		alut
		GIT_REPOSITORY https://github.com/vancegroup/freealut
		GIT_TAG		   fc814e316c2bfa6e05b723b8cc9cb276da141aae
	)
	FetchContent_GetProperties(alut)
	if(NOT alut_POPULATED)
		FetchContent_Populate(alut)

		set(OPENAL_INCLUDE_DIR ${openal_SOURCE_DIR}/include CACHE PATH "" FORCE)

		set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(BUILD_STATIC ON CACHE BOOL "" FORCE)
		set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

		set(CMAKE_FOLDER Extern/Alut)
		add_subdirectory(${alut_SOURCE_DIR} ${alut_BINARY_DIR})
	endif()
endif()

set(CMAKE_FOLDER "")

file(GLOB_RECURSE SRC *)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC})
add_library(${PROJECT_NAME} STATIC ${SRC})

target_precompile_headers(${PROJECT_NAME} PRIVATE Precomp.h)

target_compile_definitions(${PROJECT_NAME} 
	PUBLIC 
		$<$<BOOL:${VENGINE_BUILD_WITH_AUDIO}>:USE_AUDIO>
		$<$<BOOL:${VENGINE_BUILD_WITH_VULKAN}>:GLFW_INCLUDE_VULKAN>
		$<$<BOOL:${VENGINE_BUILD_WITH_VULKAN}>:USE_VULKAN>
)

target_include_directories(${PROJECT_NAME}
	PRIVATE 
		.
		${OPENGL_INCLUDE_DIR} 
		${Vulkan_INCLUDE_DIR} 
	INTERFACE
		..
)

target_link_libraries(${PROJECT_NAME} 
	PUBLIC
		Core 
		Graphics
		Physics 
		ImGUI
	PRIVATE
		TinyObjLoader
		$<$<BOOL:${VENGINE_BUILD_WITH_AUDIO}>:OpenAL>
		libglew_static # TODO: generalize this
		$<$<BOOL:${VENGINE_BUILD_WITH_AUDIO}>:alut_static>
		${OPENGL_LIBRARY} 
		$<$<BOOL:${VENGINE_BUILD_WITH_VULKAN}>:${Vulkan_LIBRARY}>
)

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})