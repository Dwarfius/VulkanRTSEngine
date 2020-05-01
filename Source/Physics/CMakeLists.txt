# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (Physics LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
	bullet3
	GIT_REPOSITORY	https://github.com/bulletphysics/bullet3
	GIT_TAG			830f0a9565b1829a07e21e2f16be2aa9966bd28c # tag for 2.89
)

FetchContent_GetProperties(bullet3)
if(NOT bullet3_POPULATED)
	FetchContent_Populate(bullet3)

	set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
	set(BUILD_BULLET3 ON CACHE BOOL "" FORCE)
	set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
	set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
	set(BUILD_ENET OFF CACHE BOOL "" FORCE)
	set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
	set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
	set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
	set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)

	set(INSTALL_CMAKE_FILES OFF CACHE BOOL "" FORCE)
	set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER Bullet3)
	add_subdirectory(${bullet3_SOURCE_DIR} ${bullet3_BINARY_DIR})
endif()

set(CMAKE_FOLDER "")

set(SRC "Precomp.cpp" "Precomp.h"
	"PhysicsEntity.cpp" "PhysicsEntity.h"
	"PhysicsWorld.cpp" "PhysicsWorld.h"
	"PhysicsShapes.cpp" "PhysicsShapes.h"
	"PhysicsDebugDrawer.cpp" "PhysicsDebugDrawer.h"
	"PhysicsCommands.cpp" "PhysicsCommands.h"
	"Utils.cpp"
)

source_group("" FILES ${SRC})

add_library(${PROJECT_NAME} STATIC ${SRC})
if (MSVC)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
	set_source_files_properties("Precomp.cpp" PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
endif(MSVC)

target_include_directories(${PROJECT_NAME} 
	PRIVATE 
		.
	PUBLIC
		# Bullet3 include dependencies, since they're not included in the target
		# TODO: should check when bullet3 reverts the target_include_directories PR in git repo
		${bullet3_SOURCE_DIR}/src
	INTERFACE
		.. # to force user to write <Physics/...>
)

target_link_libraries(${PROJECT_NAME} 
	Core 
	BulletDynamics
	BulletCollision
	LinearMath
	# BulletSoftBody
)