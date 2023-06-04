#pragma once
#include "ComponentBase.h"
#include <Core/RefCounted.h>

class Model;
class Pipeline;
class Texture;
class VisualObject;
class AssetTracker;

class VisualComponent : public SerializableComponent<VisualComponent>
{
	// declaring our own to avoid including Resource
	using ResourceId = uint32_t;
	constexpr static ResourceId kInvalId = 0;

public:
	void SetModel(Handle<Model> aModel);
	ResourceId GetModelId() const { return myModelId; }
	void SetPipeline(Handle<Pipeline> aPipeline);
	void SetTextureCount(size_t aCount);
	void SetTexture(size_t anIndex, Handle<Texture> aTexture);

	void Serialize(Serializer& aSerializer) override;

private:
	void CreateVOIfNeeded();

	std::string myModelRes;
	std::string myPipelineRes;
	std::vector<std::string> myTextureResources;
	// Not owned!
	VisualObject* myVisualObject = nullptr;

	// not serialized
	ResourceId myModelId = kInvalId;
};