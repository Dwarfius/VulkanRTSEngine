#pragma once

#include <Core/Resources/Resource.h>
#include "Animation/Skeleton.h"

class Model;
class AnimationClip;
class Texture;

// Imports all shapes declared in the .gltf file as a single Model
class GLTFImporter : public Resource
{
public:
	using Resource::Resource;
	GLTFImporter(Id anId, const std::string& aPath);

	static constexpr StaticString kDir = Resource::AssetsFolder + "objects/";

	size_t GetModelCount() const { return myModels.size(); }
	Handle<Model> GetModel(size_t anIndex) const { return myModels[anIndex]; }
	Transform GetTransform(size_t anIndex) const { return myTransforms[anIndex]; }

	size_t GetClipCount() const { return myAnimClips.size(); }
	Handle<AnimationClip> GetAnimClip(size_t anIndex) const { return myAnimClips[anIndex]; }

	size_t GetSkeletonCount() const { return mySkeletons.size(); }
	const Skeleton& GetSkeleton(size_t anIndex) const { return mySkeletons[anIndex]; }

	size_t GetTextureCount() const { return myTextures.size(); }
	Handle<Texture> GetTexture(size_t anIndex) const { return myTextures[anIndex]; }

private:
	// Determines whether this resource loads a descriptor via Serializer or a raw resorce
	bool UsesDescriptor() const final { return false; };
	void OnLoad(const std::vector<char>& aBuffer, AssetTracker&) final;

	std::vector<Handle<Model>> myModels;
	std::vector<Transform> myTransforms;
	std::vector<Handle<AnimationClip>> myAnimClips;
	std::vector<Skeleton> mySkeletons;
	std::vector<Handle<Texture>> myTextures;
};