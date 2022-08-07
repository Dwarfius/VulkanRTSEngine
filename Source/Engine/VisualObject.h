#pragma once

#include <Core/Transform.h>
#include <Core/RefCounted.h>
#include <Core/StaticVector.h>

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
	Handle<GPUModel>& GetModel() { return myModel; }
	const Handle<GPUModel>& GetModel() const { return myModel; }

	void SetPipeline(Handle<Pipeline> aPipeline);
	Handle<GPUPipeline>& GetPipeline() { return myPipeline; }
	const Handle<GPUPipeline>& GetPipeline() const { return myPipeline; }

	void SetTexture(Handle<Texture> aTexture);
	Handle<GPUTexture>& GetTexture() { return myTexture; }
	const Handle<GPUTexture>& GetTexture() const { return myTexture; }

	glm::vec3 GetCenter() const;
	float GetRadius() const;

	void SetTransform(const Transform& aTransf) { myTransf = aTransf; }
	const Transform& GetTransform() const { return myTransf; }

private:
	Transform myTransf;
	Handle<GPUModel> myModel;
	Handle<GPUPipeline> myPipeline;
	Handle<GPUTexture> myTexture;
};