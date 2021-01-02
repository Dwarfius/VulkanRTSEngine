#pragma once

#include <Core/Resources/Resource.h>
#include "Animation/Skeleton.h"

class Model;
class AnimationClip;

// Imports all shapes declared in the .gltf file as a single Model
class GLTFImporter : public Resource
{
public:
	using Resource::Resource;
	GLTFImporter(Id anId, const std::string& aPath);

	static constexpr StaticString kDir = Resource::AssetsFolder + "objects/";

	size_t GetModelCount() const { return myModels.size(); }
	Handle<Model> GetModel(size_t anIndex) const { return myModels[anIndex]; }

	size_t GetClipCount() const { return myAnimClips.size(); }
	Handle<AnimationClip> GetAnimClip(size_t anIndex) const { return myAnimClips[anIndex]; }

	size_t GetSkeletonCount() const { return mySkeletons.size(); }
	const Skeleton& GetSkeleton(size_t anIndex) const { return mySkeletons[anIndex]; }

private:
	// Determines whether this resource loads a descriptor via Serializer or a raw resorce
	bool UsesDescriptor() const override final { return false; };
	void OnLoad(const File& aFile) override final;

	std::vector<Handle<Model>> myModels;
	std::vector<Handle<AnimationClip>> myAnimClips;
	std::vector<Skeleton> mySkeletons;
};