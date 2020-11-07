# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (TinyObjLoader LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
	"tiny_obj_loader_impl.cpp" "tiny_obj_loader.h"
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		.
	INTERFACE
		..
)