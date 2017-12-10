# by Daniel Prihodko
CMAKE_MINIMUM_REQUIRED (VERSION 3.6)
PROJECT (VulkanRTSEngine)
SET (CMAKE_CXX_STANDARD 11)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

SET(TBB_DIR "" CACHE PATH "Path to root of TBB directory")

FIND_PACKAGE(Vulkan REQUIRED) 
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
FIND_PACKAGE(GLFW REQUIRED)
FIND_PACKAGE(GLM REQUIRED)
FIND_PACKAGE(ALUT REQUIRED) # FindALUT.cmake from https://github.com/rpavlik/cmake-modules/blob/master/FindALUT.cmake
FIND_PACKAGE(TBB REQUIRED) # https://github.com/01org/tbb/tree/tbb_2018/cmake

FILE(GLOB SRC "src/*.cpp" "include/*.h") 
FILE(GLOB SRC_COMP "src/Components/*.cpp" "include/Components/*.h")

ADD_EXECUTABLE(VulkanRTSEngine ${SRC} ${SRC_COMP})
if (MSVC)
   SET_TARGET_PROPERTIES(VulkanRTSEngine PROPERTIES COMPILE_FLAGS "/YuCommon.h")
   SET_SOURCE_FILES_PROPERTIES("src/Common.cpp" PROPERTIES COMPILE_FLAGS "/YcCommon.h")
   SET(EXTRA_LIBS "IPHLPAPI.lib")
endif(MSVC)

# just to make our life a little bit more organized, group files
SOURCE_GROUP("Components" FILES ${SRC_COMP})

INCLUDE_DIRECTORIES(VulkanRTSEngine include/ include/extra 
	${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR}
	${Vulkan_INCLUDE_DIR} ${ALUT_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(VulkanRTSEngine ${GLFW_LIBRARIES} ${OPENGL_LIBRARY} ${GLEW_LIBRARY}
	${Vulkan_LIBRARY} ${ALUT_LIBRARIES} ${TBB_IMPORTED_TARGETS} ${EXTRA_LIBS})

ADD_CUSTOM_COMMAND(TARGET VulkanRTSEngine PRE_BUILD COMMAND ${CMAKE_COMMAND}
					-E copy_directory ${CMAKE_SOURCE_DIR}/assets
					$<TARGET_FILE_DIR:VulkanRTSEngine>/assets)

# Copy our dlls to the executable
if (MSVC)
	if(EXISTS "${ALUT_ROOT_DIR}/bin/")
		ADD_CUSTOM_COMMAND(TARGET VulkanRTSEngine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
                    "${ALUT_ROOT_DIR}/bin/alut.dll" "$<TARGET_FILE_DIR:VulkanRTSEngine>/alut.dll")
	else()
		MESSAGE(FATAL_ERROR "ALUT_ROOT_DIR not set, can't copy dll")
	endif()
	# This is annoying, have to manually copy the the tbb dlls
	SET(TBB_DLL_PATH "${TBB_DIR}/../bin/intel64/vc14")
	ADD_CUSTOM_COMMAND(TARGET VulkanRTSEngine POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					$<$<CONFIG:Debug>:"${TBB_DLL_PATH}/tbb_debug.dll">$<$<NOT:$<CONFIG:Debug>>:"${TBB_DLL_PATH}/tbb.dll"> "$<TARGET_FILE_DIR:VulkanRTSEngine>")
endif()