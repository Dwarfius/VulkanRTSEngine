#include "Precomp.h"
#include "VisualComponent.h"

#include <Core/Resources/AssetTracker.h>
#include <Core/Resources/Serializer.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>

#include "VisualObject.h"
#include "GameObject.h"
#include "Game.h"

void VisualComponent::SetModel(Handle<Model> aModel)
{
	myModelRes = aModel->GetPath();
	CreateVOIfNeeded();
	myVisualObject->SetModel(aModel);
}

void VisualComponent::SetPipeline(Handle<Pipeline> aPipeline)
{
	myPipelineRes = aPipeline->GetPath();
	CreateVOIfNeeded();
	myVisualObject->SetPipeline(aPipeline);
}

void VisualComponent::SetTextureCount(size_t aCount)
{
	ASSERT_STR(aCount == 1, "Multi-texture support mising from VisualObject!");
	myTextureResources.resize(aCount);
}

void VisualComponent::SetTexture(size_t anIndex, Handle<Texture> aTexture)
{
	ASSERT_STR(anIndex == 0, "Multi-texture support mising from VisualObject!");
	myTextureResources[anIndex] = aTexture->GetPath();
	CreateVOIfNeeded();
	myVisualObject->SetTexture(aTexture);
}

void VisualComponent::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myTransf", myTransf);

	bool modelChanged = false;
	{
		std::string oldModel = myModelRes;
		aSerializer.Serialize("myModel", myModelRes);
		modelChanged = myModelRes != oldModel;
	}

	bool pipelineChanged = false;
	{
		std::string oldPipeline = myPipelineRes;
		aSerializer.Serialize("myPipeline", myPipelineRes);
		pipelineChanged = myPipelineRes != oldPipeline;
	}

	bool textureChanged = false;
	size_t textureSize = myTextureResources.size();
	if (Serializer::ArrayScope texturesScope{ aSerializer, "myTextures", textureSize })
	{
		myTextureResources.resize(textureSize);
		ASSERT_STR(myTextureResources.size() == 1, "Multi-texture support mising from VisualObject!");
		for (size_t i = 0; i < myTextureResources.size(); i++)
		{
			std::string oldTexture = myTextureResources[i];
			aSerializer.Serialize(Serializer::kArrayElem, myTextureResources[i]);
			textureChanged |= myTextureResources[i] != oldTexture;
		}
	}

	if (aSerializer.IsReading())
	{
		CreateVOIfNeeded();
		AssetTracker& assetTracker = Game::GetInstance()->GetAssetTracker();
		if (!myModelRes.empty() && modelChanged)
		{
			myVisualObject->SetModel(assetTracker.GetOrCreate<Model>(myModelRes));
		}
		if (!myPipelineRes.empty() && pipelineChanged)
		{
			myVisualObject->SetPipeline(assetTracker.GetOrCreate<Pipeline>(myPipelineRes));
		}
		if (!myTextureResources.empty() && textureChanged)
		{
			myVisualObject->SetTexture(assetTracker.GetOrCreate<Texture>(myTextureResources[0]));
		}
	}
}

void VisualComponent::CreateVOIfNeeded()
{
	if (myVisualObject)
	{
		return;
	}

	myOwner->CreateRenderable();
	myVisualObject = &myOwner->GetRenderable()->myVO;
}