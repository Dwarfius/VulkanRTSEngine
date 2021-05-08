#pragma once

#include <Core/RefCounted.h>
#include "Animation/Skeleton.h"

class Model;
class AnimationClip;
class Texture;
class File;

// Imports all shapes declared in the .gltf file as a single Model
class GLTFImporter
{
public:
	bool Load(const std::string& aPath);
	bool Load(const File& aFile);
	bool Load(const std::vector<char>& aBuffer, const std::string& aDir);

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
	std::vector<Handle<Model>> myModels;
	std::vector<Transform> myTransforms;
	std::vector<Handle<AnimationClip>> myAnimClips;
	std::vector<Skeleton> mySkeletons;
	std::vector<Handle<Texture>> myTextures;
};