# by Daniel Prihodko
CMAKE_MINIMUM_REQUIRED (VERSION 3.6)
PROJECT (VulkanRTSEngine)
SET (CMAKE_CXX_STANDARD 11)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# FindTBB.cmake incorrectly determines build configuration through CMAKE_BUILD_TYPE
# this forces the correct library and dependencies (Release by default)
OPTION(TBB_USE_DEBUG_BUILD "What version of TBB to use" FALSE)
SET(TBB_ROOT_DIR "" CACHE PATH "Path to root of TBB directory")

# Turning on Full Optimization to help with model loading
if(MSVC)
	# specify all optimizations
	STRING(REPLACE "/Od" "/Ox" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# turn of conflicting Run Time Checks
	STRING(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# enable inlining of everything
	STRING(REPLACE "/Ob0" "/Ob2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	# Release already has everything except for /Ox
	STRING(REPLACE "/O2" "/Ox" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
elseif(CMAKE_COMPILER_IS_GNUCC)
	# not sure, have to get my hands on a linux machine
	# need to compile with -Ofast
endif()

FIND_PACKAGE(Vulkan REQUIRED) 
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
FIND_PACKAGE(GLFW REQUIRED)
FIND_PACKAGE(GLM REQUIRED)
FIND_PACKAGE(ALUT REQUIRED) # FindALUT.cmake from https://github.com/rpavlik/cmake-modules/blob/master/FindALUT.cmake
FIND_PACKAGE(TBB REQUIRED) # FindTBB.cmake is from https://github.com/justusc/FindTBB

FILE(GLOB SRC "src/*.cpp" "include/*.h")
FILE(GLOB SRC_COMP "src/Components/*.cpp" "include/Components/*.h")

ADD_EXECUTABLE(VulkanRTSEngine ${SRC} ${SRC_COMP})
if (MSVC)
   set_target_properties(VulkanRTSEngine PROPERTIES COMPILE_FLAGS "/YuCommon.h")
   set_source_files_properties("src/Common.cpp" PROPERTIES COMPILE_FLAGS "/YcCommon.h")
endif(MSVC)

# just to make our life a little bit more organized, group files
SOURCE_GROUP("Components" FILES ${SRC_COMP})

INCLUDE_DIRECTORIES(VulkanRTSEngine include/ include/extra 
	${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR}
	${VULKAN_INCLUDE_DIR} ${ALUT_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(VulkanRTSEngine ${GLFW_LIBRARIES} ${OPENGL_LIBRARY} ${GLEW_LIBRARY}
	${VULKAN_LIBRARY} ${ALUT_LIBRARIES}  ${TBB_LIBRARIES_DEBUG} ${TBB_LIBRARIES_RELEASE})

ADD_CUSTOM_COMMAND(TARGET VulkanRTSEngine PRE_BUILD COMMAND ${CMAKE_COMMAND}
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
		SET(TBB_DLL_PATH "${TBB_ROOT_DIR}/bin/intel64/vc14")
	elseif(EXISTS "${TBB_ROOT_DIR}bin/intel64/vc12/")
		SET(TBB_DLL_PATH "${TBB_ROOT_DIR}/bin/intel64/vc12")
	elseif(EXISTS "${TBB_ROOT_DIR}/bin/intel64/vc11/")
		SET(TBB_DLL_PATH "${TBB_ROOT_DIR}/bin/intel64/vc11")
	endif()

	# if we didn't find our directory - abort
	if(NOT TBB_DLL_PATH)
		MESSAGE(FATAL_ERROR "Suitable TBB dlls not found (/bin/intel64/vc11 to vc14)")
	endif()

	ADD_CUSTOM_COMMAND(TARGET VulkanRTSEngine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					# 3.6 doesn't have IF generator expression, so have to do it the clunky way
                    $<$<CONFIG:Debug>:"${TBB_DLL_PATH}/tbb_debug.dll">$<$<NOT:$<CONFIG:Debug>>:"${TBB_DLL_PATH}/tbb.dll"> "$<TARGET_FILE_DIR:VulkanRTSEngine>/tbb_debug.dll")

	UNSET(TBB_DLL_PATH)

	if(EXISTS "${ALUT_ROOT_DIR}/bin/")
		ADD_CUSTOM_COMMAND(TARGET VulkanRTSEngine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
                    "${ALUT_ROOT_DIR}/bin/alut.dll" "$<TARGET_FILE_DIR:VulkanRTSEngine>/alut.dll")
	else()
		MESSAGE(FATAL_ERROR "ALUT_ROOT_DIR not set, can't copy dll")
	endif()
endif()