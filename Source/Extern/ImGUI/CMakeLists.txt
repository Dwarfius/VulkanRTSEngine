# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (ImGUI LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
	ImGUI
	GIT_REPOSITORY	https://github.com/ocornut/imgui
	GIT_TAG			v1.88
	GIT_SHALLOW		TRUE
	EXCLUDE_FROM_ALL # we'll manually add it
)

FetchContent_MakeAvailable(ImGUI)

# according to https://github.com/ocornut/imgui#usage we only need a couple root files
set(SRC 
	# including our config
	imconfig.h
	# including imgui sources
	${imgui_SOURCE_DIR}/imgui.cpp 
	${imgui_SOURCE_DIR}/imgui_demo.cpp 
	${imgui_SOURCE_DIR}/imgui_draw.cpp 
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	# adding headers for reading convienience
	${imgui_SOURCE_DIR}/imgui.h
	${imgui_SOURCE_DIR}/imgui_internal.h
)

add_library(${PROJECT_NAME} STATIC 
	${SRC} 
)

source_group("" FILES ${SRC})

target_include_directories(${PROJECT_NAME}
	PRIVATE
		.
	INTERFACE
		..
		${imgui_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME}
	PUBLIC
		IMGUI_USER_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h"
		IMGUI_DISABLE_OBSOLETE_FUNCTIONS
)