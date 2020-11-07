# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (STB_Image LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
	"stb_image_impl.cpp" "stb_image.h"
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		.
	INTERFACE
		..
)