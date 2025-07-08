# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (Physics LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
	bullet3
	GIT_REPOSITORY	https://github.com/bulletphysics/bullet3
	GIT_TAG			d1a4256b3a019117f2bb6cb8c63d6367aaf512e2 # fixes cmake min required version
	SOURCE_SUBDIR 	DoesNotExist # prevents add_subdir, so that we can manually configure
)

FetchContent_MakeAvailable(bullet3)

set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_ENET OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)

set(INSTALL_CMAKE_FILES OFF CACHE BOOL "" FORCE)
set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)

set(CMAKE_FOLDER Extern/Bullet3)
add_subdirectory(${bullet3_SOURCE_DIR} ${bullet3_BINARY_DIR})

set_target_properties(BulletDynamics PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})
set_target_properties(BulletCollision PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})
set_target_properties(LinearMath PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})

set(CMAKE_FOLDER "")

file(GLOB_RECURSE SRC *)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC})
add_library(${PROJECT_NAME} STATIC ${SRC})

target_precompile_headers(${PROJECT_NAME} PRIVATE Precomp.h)

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

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})