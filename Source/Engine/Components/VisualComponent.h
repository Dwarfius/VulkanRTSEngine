#pragma once
#include "ComponentBase.h"
#include <Core/Resources/Resource.h>

class Model;
class Pipeline;
class Texture;
class VisualObject;
class AssetTracker;

class VisualComponent : public SerializableComponent<VisualComponent>
{
public:
	void SetModel(Handle<Model> aModel);
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
};