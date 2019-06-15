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

	void SetModel(Handle<Model> aModel) { myModel = aModel; }
	Handle<Model> GetModel() const { return myModel; }

	void SetPipeline(Handle<Pipeline> aPipeline);
	Handle<Pipeline> GetPipeline() const { return myPipeline; }

	void SetTexture(Handle<Texture> aTexture) { myTexture = aTexture; }
	Handle<Texture> GetTexture() const { return myTexture; }

	// Returns true if object can be used for rendering 
	// (all resources have finished loading)
	bool IsValid() const;

	glm::vec3 GetCenter() const;
	float GetRadius() const;

	void SetTransform(const Transform& aTransf) { myTransf = aTransf; }
	const Transform& GetTransform() const { return myTransf; }

	UniformBlock& GetUniformBlock(size_t anIndex) const { return *myUniforms[anIndex]; }
	const vector<shared_ptr<UniformBlock>>& GetUniforms() const { return myUniforms; }
	const UniformAdapter& GetUniformAdapter(size_t anIndex) const { return *myAdapters[anIndex]; }

	void SetCategory(Category aCategory) { myCategory = aCategory; }
	Category GetCategory() const { return myCategory; }

private:
	Transform myTransf;
	Handle<Model> myModel;
	Handle<Pipeline> myPipeline;
	Handle<Texture> myTexture;

	vector<shared_ptr<UniformBlock>> myUniforms;
	vector<shared_ptr<UniformAdapter>> myAdapters;
	GameObject& myGameObject;
	Category myCategory;

	void UpdateDescriptors(const Resource* aPipelineRes);
};