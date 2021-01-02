#include "Precomp.h"
#include "GLTFImporter.h"

#include "Resources/glTF/Animation.h"
#include "Resources/glTF/Node.h"
#include "Resources/glTF/Mesh.h"
#include "Resources/glTF/Skin.h"

#include <Graphics/Resources/Model.h>

#include <Core/File.h>
#include <Core/Vertex.h>
#include <Core/Transform.h>
#include <system_error>

namespace glTF
{
	bool FindVersion(const nlohmann::json& aJson, std::string& aVersion)
	{
		const auto& assetJson = aJson.find("asset");
		if (assetJson == aJson.end())
		{
			return false;
		}

		const auto& versionJson = assetJson->find("version");
		if (versionJson == assetJson->end())
		{
			return false;
		}

		aVersion = versionJson->get<std::string>();
		return true;
	}
}

GLTFImporter::GLTFImporter(Id anId, const std::string& aPath)
	: Resource(anId, aPath)
{
}

void GLTFImporter::OnLoad(const File& aFile)
{
	// time to start learning glTF!
	// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_002_BasicGltfStructure.md
	nlohmann::json gltfJson = nlohmann::json::parse(aFile.GetBuffer(), nullptr, false);
	if (!gltfJson.is_object())
	{
		SetErrMsg("Failed to parse file!");
		return;
	}

	{
		std::string version;
		if (!glTF::FindVersion(gltfJson, version))
		{
			SetErrMsg("Failed to find asset version, unsupported version!");
			return;
		}

		if (version.compare("2.0"))
		{
			SetErrMsg("Found asset version unsupported: " + version);
			return;
		}
	}

	// TODO: at the moment we don't do full fledged scene reconstruction,
	// only exposing the related models, animation clips, mesh skins
	// as a result, we're ignoring scenes and just grabbing
	// the raw resources. To expand later!

	std::vector<glTF::Node> nodes = glTF::Node::Parse(gltfJson);
	std::vector<glTF::Buffer> buffers = glTF::Buffer::Parse(gltfJson);
	std::vector<glTF::BufferView> bufferViews = glTF::BufferView::Parse(gltfJson);
	std::vector<glTF::Accessor> accessors = glTF::Accessor::Parse(gltfJson);
	std::vector<glTF::Mesh> meshes = glTF::Mesh::Parse(gltfJson);
	std::vector<glTF::Animation> animations = glTF::Animation::Parse(gltfJson);
	std::vector<glTF::Skin> skins = glTF::Skin::Parse(gltfJson);

	{
		glTF::Mesh::ModelInputs input
		{
			buffers,
			bufferViews,
			accessors,
			meshes
		};
		glTF::Mesh::ConstructModels(input, myModels);
	}

	// we have to remap the index, as we store the bones in different hierarchy
	// (not as part of Node tree, but as a seperate Skeleton tree)
	// Key is Node index, value is Skeleton Bone index
	std::unordered_map<uint32_t, Skeleton::BoneIndex> nodeBoneMap;
	if (!skins.empty())
	{
		glTF::Skin::SkeletonInput input
		{
			buffers,
			bufferViews,
			accessors,
			nodes,
			skins
		};
		glTF::Skin::ConstructSkeletons(input, mySkeletons, nodeBoneMap);
	}

	if(!animations.empty())
	{
		glTF::Animation::AnimationClipInput input
		{
			buffers,
			bufferViews,
			accessors,
			animations,
			nodeBoneMap
		};
		glTF::Animation::ConstructAnimationClips(input, myAnimClips);
	}
}

