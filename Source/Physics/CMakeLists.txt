# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (Physics LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 14)

find_package(BULLET REQUIRED)

set(SRC "Common.cpp" "Common.h"
	"PhysicsEntity.cpp" "PhysicsEntity.h"
	"PhysicsWorld.cpp" "PhysicsWorld.h"
	"PhysicsShapes.cpp" "PhysicsShapes.h"
	"PhysicsDebugDrawer.cpp" "PhysicsDebugDrawer.h"
	"PhysicsCommands.cpp" "PhysicsCommands.h"
)

source_group("" FILES ${SRC})

add_library(${PROJECT_NAME} STATIC ${SRC})
if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuCommon.h")
   set_source_files_properties("Common.cpp" PROPERTIES COMPILE_FLAGS "/YcCommon.h")
endif(MSVC)

# do this trick later, so that consumers have to write #include <libname/...>:
# target_include_directories(libname INTERFACE include PRIVATE include/libname)
target_include_directories(${PROJECT_NAME} 
	PUBLIC 
		.
		${BULLET_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
	Core 
	${BULLET_LIBRARIES}
)