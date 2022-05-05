#pragma once

#include <Core/Transform.h>
#include <Core/RefCounted.h>

class UniformBuffer;
class Model;
class Texture;
class Pipeline;
class GPUModel;
class GPUPipeline;
class GPUTexture;

class VisualObject
{
public:
	void SetModel(Handle<Model> aModel);
	Handle<GPUModel> GetModel() const { return myModel; }

	void SetPipeline(Handle<Pipeline> aPipeline);
	Handle<GPUPipeline> GetPipeline() const { return myPipeline; }

	void SetTexture(Handle<Texture> aTexture);
	Handle<GPUTexture> GetTexture() const { return myTexture; }

	// Returns false if not all dependencies have resolved
	bool IsResolved() const { return myIsResolved; }

	glm::vec3 GetCenter() const;
	float GetRadius() const;

	void SetTransform(const Transform& aTransf) { myTransf = aTransf; }
	const Transform& GetTransform() const { return myTransf; }

	Handle<UniformBuffer> GetUniformBuffer(size_t anIndex) const { return myUniforms[anIndex]; }
	const std::vector<Handle<UniformBuffer>>& GetUniforms() const { return myUniforms; }

	bool Resolve();

private:
	Transform myTransf;
	Handle<GPUModel> myModel;
	Handle<GPUPipeline> myPipeline;
	Handle<GPUTexture> myTexture;

	// TODO: rework this to use a index-based Handle into
	// a pool of UniformBlocks
	std::vector<Handle<UniformBuffer>> myUniforms;
	bool myIsResolved = false;
	bool myIsNewPipeline = false;
};