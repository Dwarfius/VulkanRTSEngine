#include "Precomp.h"
#include "GLTFImporter.h"

#include "Resources/glTF/Animation.h"
#include "Resources/glTF/Node.h"
#include "Resources/glTF/Mesh.h"
#include "Resources/glTF/Skin.h"
#include "Resources/glTF/Texture.h"

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

bool GLTFImporter::Load(const std::string& aPath)
{
	File file(aPath);
	return file.Read() && Load(file);
}

bool GLTFImporter::Load(const File& aFile)
{
	std::string relPath = aFile.GetPath();
	size_t pos = relPath.find_last_of('/');
	if (pos != std::string::npos)
	{
		// incl last /
		relPath = relPath.substr(0, pos + 1);
	}
	else
	{
		pos = relPath.find_last_of('\\');
		if (pos != std::string::npos)
		{
			// incl last '\'
			relPath = relPath.substr(0, pos + 1);
		}
		else
		{
			relPath = std::string();
		}
	}

	return Load(aFile.GetBuffer(), relPath);
}

bool GLTFImporter::Load(const std::vector<char>& aBuffer, const std::string& aDir)
{
	myModels.clear();
	myModelNames.clear();
	myTransforms.clear();
	myAnimClips.clear();
	myClipNames.clear();
	mySkeletons.clear();
	mySkeletonNames.clear();
	myTextures.clear();
	myTextureNames.clear();

	// time to start learning glTF!
	// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_002_BasicGltfStructure.md
	nlohmann::json gltfJson = nlohmann::json::parse(aBuffer.cbegin(), aBuffer.cend(), nullptr, false);
	if (!gltfJson.is_object())
	{
		printf("Failed to parse file!");
		return false;
	}

	{
		std::string version;
		if (!glTF::FindVersion(gltfJson, version))
		{
			printf("Failed to find asset version, unsupported version!");
			return false;
		}

		if (version.compare("2.0"))
		{
			printf("Found asset version unsupported: %s", version.c_str());
			return false;
		}
	}

	// TODO: at the moment we don't do full fledged scene reconstruction,
	// only exposing the related models, animation clips, mesh skins
	// as a result, we're ignoring scenes and just grabbing
	// the raw resources. To expand later!

	std::vector<glTF::Node> nodes = glTF::Parse<glTF::Node>(gltfJson, "nodes");
	glTF::Node::UpdateWorldTransforms(nodes);

	std::vector<glTF::Buffer> buffers = glTF::Parse<glTF::Buffer>(gltfJson, "buffers", aDir);
	std::vector<glTF::BufferView> bufferViews = glTF::Parse<glTF::BufferView>(gltfJson, "bufferViews");
	std::vector<glTF::Accessor> accessors = glTF::Parse<glTF::Accessor>(gltfJson, "accessors");
	for (glTF::Accessor& accessor : accessors)
	{
		if (accessor.HasSparseBuffer())
		{
			accessor.ReconstructSparseBuffer(bufferViews, buffers);
		}
	}

	std::vector<glTF::Mesh> meshes = glTF::Parse<glTF::Mesh>(gltfJson, "meshes");
	std::vector<glTF::Skin> skins = glTF::Parse<glTF::Skin>(gltfJson, "skins");
	std::vector<glTF::Texture> textures = glTF::Parse<glTF::Texture>(gltfJson, "textures");

	{
		glTF::Mesh::ModelInputs input
		{
			buffers,
			bufferViews,
			accessors,
			nodes,
			meshes
		};
		glTF::Mesh::ConstructModels(input, myModels, myTransforms, myModelNames);
	}
	
	if (!skins.empty())
	{
		// we have to remap the index, as we store the bones in different hierarchy
		// (not as part of Node tree, but as a seperate Skeleton tree)
		// Key is Node index, value is Skeleton Bone index
		std::unordered_map<uint32_t, Skeleton::BoneIndex> nodeBoneMap;
		glTF::Skin::SkeletonInput skinInput
		{
			buffers,
			bufferViews,
			accessors,
			nodes,
			skins
		};
		glTF::Skin::ConstructSkeletons(skinInput, mySkeletons, nodeBoneMap, myTransforms, mySkeletonNames);
		
		std::vector<glTF::Animation> animations = glTF::Animation::Parse(gltfJson);
		glTF::Animation::AnimationClipInput animInput
		{
			buffers,
			bufferViews,
			accessors,
			nodes,
			animations,
			nodeBoneMap
		};
		glTF::Animation::ConstructAnimationClips(animInput, myAnimClips, myClipNames);
	}

	if (!textures.empty())
	{
		std::vector<glTF::Image> images = glTF::Parse<glTF::Image>(gltfJson, "images");
		std::vector<glTF::Sampler> samplers = glTF::Parse<glTF::Sampler>(gltfJson, "samplers");
		glTF::Texture::TextureInputs input
		{
			buffers,
			bufferViews,
			accessors,
			images,
			samplers,
			textures,
			aDir
		};
		glTF::Texture::ConstructTextures(input, myTextures, myTextureNames);
	}
	return true;
}

