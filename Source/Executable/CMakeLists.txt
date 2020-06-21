# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (VEngineExe LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

file(GLOB SRC "*.cpp" "*.h")
source_group("" FILES ${SRC})

add_executable(${PROJECT_NAME} ${SRC})

target_precompile_headers(${PROJECT_NAME} PRIVATE Precomp.h)

target_link_libraries(${PROJECT_NAME} Engine)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir)")
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})