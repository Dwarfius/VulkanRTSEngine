# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)
project (Engine LANGUAGES CXX)

include(FetchContent)

if(VENGINE_BUILD_WITH_VULKAN)
	find_package(Vulkan REQUIRED)
endif() 

find_package(OpenGL REQUIRED)

# we're going to grab a fork of glew which has proper CMakeLists and headers generated
FetchContent_Declare(
	glew
	GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
	GIT_TAG        glew-cmake-2.2.0
)

FetchContent_GetProperties(glew)
if(NOT glew_POPULATED)
	FetchContent_Populate(glew)

	# enable our configuration
	set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_SINGLE_CONTEXT ON CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(glew-cmake_BUILD_MULTI_CONTEXT OFF CACHE BOOL "" FORCE)

	set(CMAKE_FOLDER Extern/GLEW)
	add_subdirectory(${glew_SOURCE_DIR} ${glew_BINARY_DIR})
endif()

if(VENGINE_BUILD_WITH_AUDIO)
	# freealut is old, last commit was June 2014
	FetchContent_Declare(
		alut
		GIT_REPOSITORY https://github.com/vancegroup/freealut
		GIT_TAG		   fc814e316c2bfa6e05b723b8cc9cb276da141aae
	)
	FetchContent_GetProperties(alut)
	if(NOT alut_POPULATED)
		FetchContent_Populate(alut)

		set(OPENAL_INCLUDE_DIR ${openal_SOURCE_DIR}/include CACHE PATH "" FORCE)

		set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(BUILD_STATIC ON CACHE BOOL "" FORCE)
		set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

		set(CMAKE_FOLDER Extern/Alut)
		add_subdirectory(${alut_SOURCE_DIR} ${alut_BINARY_DIR})
	endif()
endif()

set(CMAKE_FOLDER "")

set(SRC "Precomp.h"
	"Audio.cpp" "Audio.h"
	"Game.cpp" "Game.h"
	"GameObject.cpp" "GameObject.h"
	"GameTaskManager.cpp" "GameTaskManager.h"
	"Input.cpp" "Input.h"
	"RenderThread.cpp" "RenderThread.h"
	"Terrain.cpp" "Terrain.h"
	"VisualObject.cpp" "VisualObject.h"
)

set(SRC_ANIMATION
	"Animation/Skeleton.cpp" "Animation/Skeleton.h"
	"Animation/AnimationClip.cpp" "Animation/AnimationClip.h"
	"Animation/AnimationController.cpp" "Animation/AnimationController.h"
	"Animation/AnimationSystem.cpp" "Animation/AnimationSystem.h"
	"Animation/SkinnedVerts.h"
)

set(SRC_COMP 
	"Components/ComponentBase.cpp" "Components/ComponentBase.h"
	"Components/PhysicsComponent.cpp" "Components/PhysicsComponent.h"
	"Components/VisualComponent.cpp" "Components/VisualComponent.h"
)

set(SRC_GRAPHICS 
	"Graphics/NamedFrameBuffers.h"
)

set(SRC_GRAPHICS_RENDERPASSES
	"Graphics/RenderPasses/GenericRenderPasses.cpp" "Graphics/RenderPasses/GenericRenderPasses.h"
)

set(SRC_GRAPHICS_GL 
	"Graphics/GL/FrameBufferGL.cpp" "Graphics/GL/FrameBufferGL.h"
	"Graphics/GL/GraphicsGL.cpp" "Graphics/GL/GraphicsGL.h"
	"Graphics/GL/ModelGL.cpp" "Graphics/GL/ModelGL.h"
	"Graphics/GL/PipelineGL.cpp" "Graphics/GL/PipelineGL.h"
	"Graphics/GL/RenderPassJobGL.cpp" "Graphics/GL/RenderPassJobGL.h"
	"Graphics/GL/ShaderGL.cpp" "Graphics/GL/ShaderGL.h"
	"Graphics/GL/TextureGL.cpp" "Graphics/GL/TextureGL.h"
	"Graphics/GL/UniformBufferGL.cpp" "Graphics/GL/UniformBufferGL.h"
)

set(SRC_GRAPHICS_ADAPTERS 
	"Graphics/Adapters/AdapterSourceData.h"
	"Graphics/Adapters/CameraAdapter.cpp" "Graphics/Adapters/CameraAdapter.h"
	"Graphics/Adapters/ObjectMatricesAdapter.cpp" "Graphics/Adapters/ObjectMatricesAdapter.h"
	"Graphics/Adapters/SkeletonAdapter.cpp" "Graphics/Adapters/SkeletonAdapter.h"
	"Graphics/Adapters/TerrainAdapter.cpp" "Graphics/Adapters/TerrainAdapter.h"
)

set(SRC_RESOURCES
	"Resources/GLTFImporter.cpp" "Resources/GLTFImporter.h"
	"Resources/ImGUISerializer.cpp" "Resources/ImGUISerializer.h"
	"Resources/OBJImporter.cpp" "Resources/OBJImporter.h"
)

set(SRC_RESOURCES_GLTF
	"Resources/glTF/Accessor.cpp" "Resources/glTF/Accessor.h"
	"Resources/glTF/Animation.cpp" "Resources/glTF/Animation.h"
	"Resources/glTF/Buffer.h"
	"Resources/glTF/BufferView.h"
	"Resources/glTF/Common.h"
	"Resources/glTF/Mesh.cpp" "Resources/glTF/Mesh.h"
	"Resources/glTF/Node.h"
	"Resources/glTF/Skin.cpp" "Resources/glTF/Skin.h"
	"Resources/glTF/Texture.cpp" "Resources/glTF/Texture.h"
)

set(SRC_SYSTEMS
	"Systems/ProfilerUI.cpp" "Systems/ProfilerUI.h"
)

set(SRC_SYSTEMS_IMGUI
	"Systems/ImGUI/ImGUISystem.cpp" "Systems/ImGUI/ImGUISystem.h"
	"Systems/ImGUI/ImGUIGLFWImpl.cpp" "Systems/ImGUI/ImGUIGLFWImpl.h"
	"Systems/ImGUI/ImGUIRendering.cpp" "Systems/ImGUI/ImGUIRendering.h"
)

set(SRC_UIWIDGETS
	"UIWidgets/EntitiesWidget.cpp" "UIWidgets/EntitiesWidget.h"
	"UIWidgets/FileDialog.cpp" "UIWidgets/FileDialog.h"
	"UIWidgets/GltfImportDialog.cpp" "UIWidgets/GltfImportDialog.h"
	"UIWidgets/ObjImportDialog.cpp" "UIWidgets/ObjImportDialog.h"
	"UIWidgets/PipelineCreateDialog.cpp" "UIWidgets/PipelineCreateDialog.h"
	"UIWidgets/ShaderCreateDialog.cpp" "UIWidgets/ShaderCreateDialog.h"
	"UIWidgets/TextureImportDialog.cpp" "UIWidgets/TextureImportDialog.h"
	"UIWidgets/TopBar.cpp" "UIWidgets/TopBar.h"
)

if(VENGINE_BUILD_WITH_VULKAN)
	set(SRC_GRAPHICS_VK "Graphics/VK/GraphicsVK.cpp" "Graphics/VK/GraphicsVK.h"
	)
endif()

add_library(${PROJECT_NAME} STATIC 
	${SRC} 
		${SRC_ANIMATION}
		${SRC_COMP} 
		${SRC_GRAPHICS} 
			${SRC_GRAPHICS_RENDERPASSES}
			${SRC_GRAPHICS_GL} 
			${SRC_GRAPHICS_VK}
			${SRC_GRAPHICS_ADAPTERS}
		${SRC_RESOURCES}
			${SRC_RESOURCES_GLTF}
		${SRC_SYSTEMS}
			${SRC_SYSTEMS_IMGUI}
		${SRC_UIWIDGETS}
)

# just to make our life a little bit more organized, group files
source_group("" FILES ${SRC})
source_group(Animation FILES ${SRC_ANIMATION})
source_group(Components FILES ${SRC_COMP})
source_group(Graphics FILES ${SRC_GRAPHICS})
source_group(Graphics\\Adapters FILES ${SRC_GRAPHICS_ADAPTERS})
source_group(Graphics\\RenderPasses FILES ${SRC_GRAPHICS_RENDERPASSES})
source_group(Graphics\\GL FILES ${SRC_GRAPHICS_GL})
if(VENGINE_BUILD_WITH_VULKAN)
	source_group(Graphics\\VK FILES ${SRC_GRAPHICS_VK})
endif()
source_group(Resources FILES ${SRC_RESOURCES})
source_group(Resources\\glTF FILES ${SRC_RESOURCES_GLTF})
source_group(Systems FILES ${SRC_SYSTEMS})
source_group(Systems\\ImGUI FILES ${SRC_SYSTEMS_IMGUI})
source_group(UIWidgets FILES ${SRC_UIWIDGETS})

target_precompile_headers(${PROJECT_NAME} PRIVATE Precomp.h)

target_compile_definitions(${PROJECT_NAME} 
	PUBLIC 
		$<$<BOOL:${VENGINE_BUILD_WITH_AUDIO}>:USE_AUDIO>
		$<$<BOOL:${VENGINE_BUILD_WITH_VULKAN}>:GLFW_INCLUDE_VULKAN>
		$<$<BOOL:${VENGINE_BUILD_WITH_VULKAN}>:USE_VULKAN>
)

target_include_directories(${PROJECT_NAME}
	PRIVATE 
		.
		${OPENGL_INCLUDE_DIR} 
		${Vulkan_INCLUDE_DIR} 
	INTERFACE
		..
)

target_link_libraries(${PROJECT_NAME} 
	PUBLIC
		Core 
		Graphics
		Physics 
		ImGUI
	PRIVATE
		TinyObjLoader
		$<$<BOOL:${VENGINE_BUILD_WITH_AUDIO}>:OpenAL>
		libglew_static # TODO: generalize this
		$<$<BOOL:${VENGINE_BUILD_WITH_AUDIO}>:alut_static>
		${OPENGL_LIBRARY} 
		$<$<BOOL:${VENGINE_BUILD_WITH_VULKAN}>:${Vulkan_LIBRARY}>
)

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ${VENGINE_ENABLE_UNITYBUILDS})