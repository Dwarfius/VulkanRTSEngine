# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (Engine LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

include(FetchContent)

# use this for https://github.com/ocornut/imgui and others, potentially

find_package(Vulkan REQUIRED) 
find_package(OpenGL REQUIRED)
find_package(ALUT REQUIRED) # FindALUT.cmake from https://github.com/rpavlik/cmake-modules/blob/master/FindALUT.cmake

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
	add_subdirectory(${glew_SOURCE_DIR} ${glew_BINARY_DIR})
endif()

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

set(SRC_GRAPHICS "Graphics/UniformAdapter.cpp" "Graphics/UniformAdapter.h"
	"Graphics/UniformAdapterRegister.cpp" "Graphics/UniformAdapterRegister.h")

set(SRC_GRAPHICS_GL "Graphics/GL/GraphicsGL.cpp" "Graphics/GL/GraphicsGL.h"
	"Graphics/GL/ModelGL.cpp" "Graphics/GL/ModelGL.h"
	"Graphics/GL/PipelineGL.cpp" "Graphics/GL/PipelineGL.h"
	"Graphics/GL/ShaderGL.cpp" "Graphics/GL/ShaderGL.h"
	"Graphics/GL/TextureGL.cpp" "Graphics/GL/TextureGL.h"
	"Graphics/GL/UniformBufferGL.cpp" "Graphics/GL/UniformBufferGL.h"
)

set(SRC_GRAPHICS_VK "Graphics/VK/GraphicsVK.cpp" "Graphics/VK/GraphicsVK.h"
)

add_library(${PROJECT_NAME} STATIC 
	${SRC} 
		${SRC_COMP} 
		${SRC_GRAPHICS} 
			${SRC_GRAPHICS_GL} 
			${SRC_GRAPHICS_VK}
)

if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
   set_source_files_properties("Precomp.cpp" PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
endif(MSVC)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})
source_group(Components FILES ${SRC_COMP})
source_group(Graphics FILES ${SRC_GRAPHICS})
source_group(Graphics\\GL FILES ${SRC_GRAPHICS_GL})
source_group(Graphics\\VK FILES ${SRC_GRAPHICS_VK})

target_compile_definitions(${PROJECT_NAME} 
	PUBLIC 
		GLFW_INCLUDE_VULKAN
)

target_include_directories(${PROJECT_NAME}
	PUBLIC 
		. 
	PRIVATE 
		${OPENGL_INCLUDE_DIR} 
		${Vulkan_INCLUDE_DIR} 
		${ALUT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
	Core 
	Physics 
	${OPENGL_LIBRARY} 
	libglew_static # TODO: generalize this
	${Vulkan_LIBRARY} 
	${ALUT_LIBRARIES}
)