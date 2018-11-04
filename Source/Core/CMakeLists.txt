# by Daniel Prihodko
cmake_minimum_required (VERSION 3.8)
project (Core LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

find_package(GLFW REQUIRED)
find_package(GLM REQUIRED)
find_package(TBB REQUIRED) # https://github.com/01org/tbb/tree/tbb_2018/cmake

set(SRC "Precomp.cpp" "Precomp.h"
	"Camera.cpp" "Camera.h"
	"Transform.cpp" "Transform.h"
	"UID.cpp" "UID.h"
	"RefCounted.cpp" "RefCounted.h"
	"RWBuffer.h"
	"Utils.cpp" "Utils.h"
	"Vertex.h"
) 

set(SRC_THREADING "Threading/AssertMutex.cpp" "Threading/AssertMutex.h"
	"Threading/AssertRWMutex.cpp" "Threading/AssertRWMutex.h"
)

set(SRC_GRAPHICS "Graphics/AssetTracker.cpp" "Graphics/AssetTracker.h"
	"Graphics/Descriptor.cpp" "Graphics/Descriptor.h"
	"Graphics/Graphics.cpp" "Graphics/Graphics.h"
	"Graphics/GraphicsTypes.h"
	"Graphics/Model.cpp" "Graphics/Model.h"
	"Graphics/Pipeline.cpp" "Graphics/Pipeline.h"
	"Graphics/Resource.cpp" "Graphics/Resource.h"
	"Graphics/Shader.cpp" "Graphics/Shader.h"
	"Graphics/Texture.cpp" "Graphics/Texture.h"
	"Graphics/UniformBlock.cpp" "Graphics/UniformBlock.h"
)

set(SRC_DEBUG "Debug/Assert.cpp" "Debug/Assert.h"
)

add_library(${PROJECT_NAME} STATIC ${SRC} ${SRC_THREADING} ${SRC_DEBUG} ${SRC_GRAPHICS})
if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
   set_source_files_properties("Precomp.cpp" PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
   set(EXTRA_LIBS "IPHLPAPI.lib") # for UID class
endif(MSVC)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})
source_group(Threading FILES ${SRC_THREADING})
source_group(Debug FILES ${SRC_DEBUG})
source_group(Graphics FILES ${SRC_GRAPHICS})

target_compile_definitions(${PROJECT_NAME} 
	PUBLIC 
		NOMINMAX # to prevent tbb to carry over min/max macros from win headers
		GLM_FORCE_DEPTH_ZERO_TO_ONE
		ASSERT_MUTEX # enables utility mutex that asserts on concurrent use
		# ENABLE_ASSERTS # uncomment this to force asserts
)

target_include_directories(${PROJECT_NAME}
	PUBLIC 
		. 
		${GLM_INCLUDE_DIR} 
		${GLFW_INCLUDE_DIR}
	PRIVATE 
		Extra
#		${GLFW_INCLUDE_DIR} TODO: look into hiding it Core
)

target_link_libraries(${PROJECT_NAME} 
	${GLFW_LIBRARIES} 
	${TBB_IMPORTED_TARGETS} 
	${EXTRA_LIBS}
)