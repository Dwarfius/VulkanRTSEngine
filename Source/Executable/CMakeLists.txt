# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (VEngineExe LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

file(GLOB SRC "*.cpp" "*.h")
source_group("" FILES ${SRC})

add_executable(${PROJECT_NAME} ${SRC} ${SRC_COMP})
if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuCommon.h")
   set_source_files_properties("Common.cpp" PROPERTIES COMPILE_FLAGS "/YcCommon.h")
endif(MSVC)

target_link_libraries(${PROJECT_NAME} Engine)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir)")

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND}
					-E copy_directory ${CMAKE_SOURCE_DIR}/assets
					$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)

# Copy our dlls to the executable
if (MSVC)
	if(EXISTS "${ALUT_ROOT_DIR}/bin/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
                    "${ALUT_ROOT_DIR}/bin/alut.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/alut.dll")
	else()
		message(FATAL_ERROR "ALUT_ROOT_DIR not set, can't copy ALUT dlls")
	endif()
	# This is annoying, have to manually copy the the tbb dlls
	set(TBB_DLL_PATH "${TBB_DIR}/../bin/intel64/vc14")
	if(EXISTS "${TBB_DLL_PATH}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					$<$<CONFIG:Debug>:"${TBB_DLL_PATH}/tbb_debug.dll">$<$<NOT:$<CONFIG:Debug>>:"${TBB_DLL_PATH}/tbb.dll"> "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
	else()
		message(FATAL_ERROR "TBB_DIR not set, can't copy TBB dlls") 
	endif()
endif()