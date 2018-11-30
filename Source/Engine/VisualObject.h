#pragma once

#include "Transform.h"
#include "RefCounted.h"

#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"
#include "Graphics/Model.h"

class UniformBlock;
class GameObject;
class UniformAdapter;

class VisualObject
{
public:
	VisualObject(GameObject& aGO);

	void SetModel(Handle<Model> aModel);
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

private:
	Transform myTransf;
	Handle<Model> myModel;
	Handle<Pipeline> myPipeline;
	Handle<Texture> myTexture;

	vector<shared_ptr<UniformBlock>> myUniforms;
	vector<shared_ptr<UniformAdapter>> myAdapters;
	GameObject& myGameObject;

	void UpdateCenter(const Resource* aModelRes);
	void UpdateDescriptors(const Resource* aPipelineRes);
};