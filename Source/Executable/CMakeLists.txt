# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (VEngineExe LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

file(GLOB SRC "*.cpp" "*.h")
source_group("" FILES ${SRC})

add_executable(${PROJECT_NAME} ${SRC})
if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
   set_source_files_properties("Precomp.cpp" PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
endif(MSVC)

target_link_libraries(${PROJECT_NAME} Engine)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir)")