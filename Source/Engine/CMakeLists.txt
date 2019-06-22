# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (Engine LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

include(FetchContent)

# use this for https://github.com/ocornut/imgui and others, potentially

if(VENGINE_BUILD_WITH_VULKAN)
	find_package(Vulkan REQUIRED)
endif() 

find_package(OpenGL REQUIRED)

# we're going to grab a fork of glew which has proper CMakeLists and headers generated
FetchContent_Declare(
	glew
	GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
	GIT_TAG        6dbc9407729d653e4d254b772c66af138b00617c # tag for 2.1.0 mirror
)

FetchContent_GetProperties(glew)
if(NOT glew_POPULATED)
	FetchContent_Populate(glew)

	# enable our configuration
	set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_SINGLE_CONTEXT ON CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_MULTI_CONTEXT OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER GLEW)
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

		set(CMAKE_FOLDER Alut)
		add_subdirectory(${alut_SOURCE_DIR} ${alut_BINARY_DIR})
	endif()
endif()

set(CMAKE_FOLDER "")

set(SRC "Precomp.cpp" "Precomp.h"
	"Audio.cpp" "Audio.h"
	"Game.cpp" "Game.h"
	"GameObject.cpp" "GameObject.h"
	"GameTaskManager.cpp" "GameTaskManager.h"
	"Input.cpp" "Input.h" # TODO: should I move it to Core?
	"RenderThread.cpp" "RenderThread.h"
	"Terrain.cpp" "Terrain.h"
	"VisualObject.cpp" "VisualObject.h"
)

set(SRC_COMP "Components/ComponentBase.cpp" "Components/ComponentBase.h"
	"Components/EditorMode.cpp" "Components/EditorMode.h"
	"Components/PhysicsComponent.cpp" "Components/PhysicsComponent.h"
)

set(SRC_GRAPHICS 
)

set(SRC_GRAPHICS_RENDERPASSES
	"Graphics/RenderPasses/GenericRenderPasses.cpp" "Graphics/RenderPasses/GenericRenderPasses.h"
)

set(SRC_GRAPHICS_GL "Graphics/GL/GraphicsGL.cpp" "Graphics/GL/GraphicsGL.h"
	"Graphics/GL/ModelGL.cpp" "Graphics/GL/ModelGL.h"
	"Graphics/GL/PipelineGL.cpp" "Graphics/GL/PipelineGL.h"
	"Graphics/GL/RenderPassJobGL.cpp" "Graphics/GL/RenderPassJobGL.h"
	"Graphics/GL/ShaderGL.cpp" "Graphics/GL/ShaderGL.h"
	"Graphics/GL/TextureGL.cpp" "Graphics/GL/TextureGL.h"
	"Graphics/GL/UniformBufferGL.cpp" "Graphics/GL/UniformBufferGL.h"
)

set(SRC_GRAPHICS_ADAPTERS 
	"Graphics/Adapters/CameraAdapter.cpp" "Graphics/Adapters/CameraAdapter.h"
	"Graphics/Adapters/TerrainAdapter.cpp" "Graphics/Adapters/TerrainAdapter.h"
	"Graphics/Adapters/ObjectMatricesAdapter.cpp" "Graphics/Adapters/ObjectMatricesAdapter.h"
	"Graphics/Adapters/UniformAdapter.h"
	"Graphics/Adapters/UniformAdapterRegister.cpp" "Graphics/Adapters/UniformAdapterRegister.h"
)

if(VENGINE_BUILD_WITH_VULKAN)
	set(SRC_GRAPHICS_VK "Graphics/VK/GraphicsVK.cpp" "Graphics/VK/GraphicsVK.h"
	)
endif()

add_library(${PROJECT_NAME} STATIC 
	${SRC} 
		${SRC_COMP} 
		${SRC_GRAPHICS} 
			${SRC_GRAPHICS_RENDERPASSES}
			${SRC_GRAPHICS_GL} 
			${SRC_GRAPHICS_VK}
			${SRC_GRAPHICS_ADAPTERS}
)

if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
   set_source_files_properties("Precomp.cpp" PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
endif(MSVC)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})
source_group(Components FILES ${SRC_COMP})
source_group(Graphics FILES ${SRC_GRAPHICS})
source_group(Graphics\\Adapters FILES ${SRC_GRAPHICS_ADAPTERS})
source_group(Graphics\\RenderPasses FILES ${SRC_GRAPHICS_RENDERPASSES})
source_group(Graphics\\GL FILES ${SRC_GRAPHICS_GL})
if(VENGINE_BUILD_WITH_VULKAN)
	source_group(Graphics\\VK FILES ${SRC_GRAPHICS_VK})
endif()

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
	Core 
	Graphics
	Physics 
	$<$<BOOL:${VENGINE_BUILD_WITH_AUDIO}>:OpenAL>
	libglew_static # TODO: generalize this
	$<$<BOOL:${VENGINE_BUILD_WITH_AUDIO}>:alut_static>
	${OPENGL_LIBRARY} 
	$<$<BOOL:${VENGINE_BUILD_WITH_VULKAN}>:${Vulkan_LIBRARY}>
)