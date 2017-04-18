# by Daniel Prihodko
cmake_minimum_required (VERSION 3.6)
project (VulkanRTSEngine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

# FindTBB.cmake incorrectly determines build configuration through CMAKE_BUILD_TYPE
# this forces the correct library and dependencies (Release by default)
option(TBB_USE_DEBUG_BUILD "What version of TBB to use" FALSE)
set(TBB_ROOT_DIR "" CACHE PATH "Path to root of TBB directory")

# Turning on Full Optimization to help with model loading
if(MSVC)
	# specify all optimizations
	string(REPLACE "/Od" "/Ox" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# turn of conflicting Run Time Checks
	string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# enable inlining of everything
	string(REPLACE "/Ob0" "/Ob2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# Release already has everything except for /Ox
	string(REPLACE "/O2" "/Ox" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
elseif(CMAKE_COMPILER_IS_GNUCC)
	# not sure, have to get my hands on a linux machine
	# need to compile with -Ofast
endif()

find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLFW REQUIRED)
find_package(GLM REQUIRED)
find_package(ALUT REQUIRED) # FindALUT.cmake from https://github.com/rpavlik/cmake-modules/blob/master/FindALUT.cmake
find_package(TBB REQUIRED) # FindTBB.cmake is from https://github.com/justusc/FindTBB

file(GLOB SRC "src/*.cpp")
file(GLOB SRC_COMP "src/Components/*.cpp")
file(GLOB INC "include/*.h")
file(GLOB INC_COMP "include/Components/*.h")

add_executable(VulkanRTSEngine 
	# Our own sources...
	${SRC} ${SRC_COMP}
	# headers
	${INC} ${INC_COMP})

# just to make our life a little bit more organized, group files
source_group("Components" FILES ${SRC_COMP}	${INC_COMP})

include_directories(VulkanRTSEngine include/ include/extra 
	${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR}
	${VULKAN_INCLUDE_DIR} ${ALUT_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS})
target_link_libraries(VulkanRTSEngine ${GLFW_LIBRARIES} ${OPENGL_LIBRARY} ${GLEW_LIBRARY}
	${VULKAN_LIBRARY} ${ALUT_LIBRARIES} ${TBB_LIBRARIES_DEBUG} ${TBB_LIBRARIES_RELEASE})

add_custom_command(TARGET VulkanRTSEngine PRE_BUILD COMMAND ${CMAKE_COMMAND}
					-E copy_directory ${CMAKE_SOURCE_DIR}/assets
					$<TARGET_FILE_DIR:VulkanRTSEngine>/assets)

# Copy our dlls to the executable
if (MSVC)
	# Intel's TBB (2017, update 5) has an issue that it fetches the tbb_debug.dll constantly
	# no matter which .lib used (debug or release). I've tried manually building the sources,
	# but the issue remained. To avoid using a debug dll with internal runtime checks,
	# we copy the required dll with the name change.

	# looking for TBB dlls folder
	if(EXISTS "${TBB_ROOT_DIR}/bin/intel64/vc14/")
		set(TBB_DLL_PATH "${TBB_ROOT_DIR}/bin/intel64/vc14")
	elseif(EXISTS "${TBB_ROOT_DIR}bin/intel64/vc12/")
		set(TBB_DLL_PATH "${TBB_ROOT_DIR}/bin/intel64/vc12")
	elseif(EXISTS "${TBB_ROOT_DIR}/bin/intel64/vc11/")
		set(TBB_DLL_PATH "${TBB_ROOT_DIR}/bin/intel64/vc11")
	endif()

	# if we didn't find our directory - abort
	if(NOT TBB_DLL_PATH)
		message(FATAL_ERROR "Suitable TBB dlls not found (/bin/intel64/vc11 to vc14)")
	endif()

	add_custom_command(TARGET VulkanRTSEngine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					# 3.6 doesn't have IF generator expression, so have to do it the clunky way
                    $<$<CONFIG:Debug>:"${TBB_DLL_PATH}/tbb_debug.dll">$<$<NOT:$<CONFIG:Debug>>:"${TBB_DLL_PATH}/tbb.dll"> "$<TARGET_FILE_DIR:VulkanRTSEngine>/tbb_debug.dll")

	unset(TBB_DLL_PATH)

	if(EXISTS "${ALUT_ROOT_DIR}/bin/")
		add_custom_command(TARGET VulkanRTSEngine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
                    "${ALUT_ROOT_DIR}/bin/alut.dll" "$<TARGET_FILE_DIR:VulkanRTSEngine>/alut.dll")
	else()
		message(FATAL_ERROR "ALUT_ROOT_DIR not set, can't copy dll")
	endif()
endif()