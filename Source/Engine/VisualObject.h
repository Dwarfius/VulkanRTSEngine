#pragma once

#include <Core/Transform.h>
#include <Core/RefCounted.h>

class UniformBlock;
class GameObject;
class UniformAdapter;
class Model;
class Texture;
class Pipeline;
class Resource;
class GPUResource;

class VisualObject
{
public:
	enum class Category : char
	{
		GameObject,
		Terrain
	};

public:
	VisualObject(GameObject& aGO);

	void SetModel(Handle<Model> aModel);
	Handle<GPUResource> GetModel() const { return myModel; }

	void SetPipeline(Handle<Pipeline> aPipeline);
	Handle<GPUResource> GetPipeline() const { return myPipeline; }

	void SetTexture(Handle<Texture> aTexture);
	Handle<GPUResource> GetTexture() const { return myTexture; }

	// Returns true if object can be used for rendering 
	// (all resources have finished loading)
	bool IsValid() const;

	glm::vec3 GetCenter() const;
	float GetRadius() const;

	void SetTransform(const Transform& aTransf) { myTransf = aTransf; }
	const Transform& GetTransform() const { return myTransf; }

	UniformBlock& GetUniformBlock(size_t anIndex) const { return *myUniforms[anIndex]; }
	const std::vector<std::shared_ptr<UniformBlock>>& GetUniforms() const { return myUniforms; }
	const UniformAdapter& GetUniformAdapter(size_t anIndex) const { return *myAdapters[anIndex]; }

	void SetCategory(Category aCategory) { myCategory = aCategory; }
	Category GetCategory() const { return myCategory; }

private:
	Transform myTransf;
	Handle<GPUResource> myModel;
	Handle<GPUResource> myPipeline;
	Handle<GPUResource> myTexture;

	std::vector<std::shared_ptr<UniformBlock>> myUniforms;
	std::vector<std::shared_ptr<UniformAdapter>> myAdapters;
	GameObject& myGameObject;
	Category myCategory;

	void UpdateDescriptors(const Resource* aPipelineRes);
};