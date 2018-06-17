# by Daniel Prihodko
cmake_minimum_required (VERSION 3.6)
project(VEngine LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_subdirectory(Source/Engine)
add_subdirectory(Source/Physics)
add_subdirectory(Source/Executable)