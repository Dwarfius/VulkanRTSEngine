# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (Physics LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 14)

find_package(BULLET REQUIRED)
find_package(GLM REQUIRED)
find_package(TBB REQUIRED) # https://github.com/01org/tbb/tree/tbb_2018/cmake

set(SRC "Common.cpp" "PhysicsEntity.cpp" "PhysicsWorld.cpp" 
	"PhysicsShapes.cpp" "PhysicsDebugDrawer.cpp" "PhysicsCommands.cpp")
set(HEADERS "Common.h" "PhysicsEntity.h" "PhysicsWorld.h" 
	"PhysicsShapes.h" "PhysicsDebugDrawer.h" "PhysicsCommands.h")
source_group("" FILES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} STATIC ${SRC} ${HEADERS})
if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuCommon.h")
   set_source_files_properties("Common.cpp" PROPERTIES COMPILE_FLAGS "/YcCommon.h")
endif(MSVC)

# do this trick later, so that consumers have to write #include <libname/...>:
# target_include_directories(libname INTERFACE include PRIVATE include/libname)
target_include_directories(${PROJECT_NAME} 
							PUBLIC . ${GLM_INCLUDE_DIR} ${BULLET_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${BULLET_LIBRARIES} ${TBB_IMPORTED_TARGETS})