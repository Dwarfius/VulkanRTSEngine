# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (Engine LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 14)

# use this for https://github.com/ocornut/imgui and others, potentially
# https://stackoverflow.com/questions/38006584/how-to-clone-and-integrate-external-from-git-cmake-project-into-local-one

find_package(Vulkan REQUIRED) 
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(ALUT REQUIRED) # FindALUT.cmake from https://github.com/rpavlik/cmake-modules/blob/master/FindALUT.cmake

file(GLOB SRC "Common.cpp" "Common.h"
	"Audio.cpp" "Audio.h"
	"Game.cpp" "Game.h"
	"GameObject.cpp" "GameObject.h"
	"GameTaskManager.cpp" "GameTaskManager.h"
	"GraphicsGL.cpp" "GraphicsGL.h"
	"GraphicsVK.cpp" "GraphicsVK.h"
	"Input.cpp" "Input.h" # TODO: should I move it to Core?
	"RenderThread.cpp" "RenderThread.h"
	"Terrain.cpp" "Terrain.h"
)

set(SRC_COMP "Components/ComponentBase.cpp" "Components/ComponentBase.h"
	"Components/EditorMode.cpp" "Components/EditorMode.h"
	"Components/Missile.cpp" "Components/Missile.h"
	"Components/PhysicsComponent.cpp" "Components/PhysicsComponent.h"
	"Components/PlayerTank.cpp" "Components/PlayerTank.h"
	"Components/Renderer.h"
	"Components/Tank.cpp" "Components/Tank.h"
	"Components/TankArenaGameMode.cpp" "Components/TankArenaGameMode.h"
)

add_library(${PROJECT_NAME} STATIC ${SRC} ${SRC_COMP})
if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuCommon.h")
   set_source_files_properties("Common.cpp" PROPERTIES COMPILE_FLAGS "/YcCommon.h")
endif(MSVC)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})
source_group("Components" FILES ${SRC_COMP})

target_compile_definitions(${PROJECT_NAME} 
	PUBLIC 
		GLFW_INCLUDE_VULKAN
)

target_include_directories(${PROJECT_NAME}
	PUBLIC 
		. 
	PRIVATE 
		${OPENGL_INCLUDE_DIR} 
		${GLEW_INCLUDE_DIR} 
		${Vulkan_INCLUDE_DIR} 
		${ALUT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
	Core 
	Physics 
	${OPENGL_LIBRARY} 
	${GLEW_LIBRARY} 
	${Vulkan_LIBRARY} 
	${ALUT_LIBRARIES}
)