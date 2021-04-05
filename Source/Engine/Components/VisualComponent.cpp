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
#include "Resources/GLTFImporter.h"
#include "Resources/OBJImporter.h"

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
	if (Serializer::Scope transfScope = aSerializer.SerializeObject("myTransf"))
	{
		myTransf.Serialize(aSerializer);
	}
	aSerializer.Serialize("myModel", myModelRes);
	aSerializer.Serialize("myPipeline", myPipelineRes);
	if(Serializer::Scope texturesScope = aSerializer.SerializeArray("myTextures", myTextureResources))
	{
		ASSERT_STR(myTextureResources.size() == 1, "Multi-texture support mising from VisualObject!");
		for (size_t i = 0; i < myTextureResources.size(); i++)
		{
			aSerializer.Serialize(i, myTextureResources[i]);
		}
	}

	if (aSerializer.IsReading())
	{
		CreateVOIfNeeded();
		AssetTracker& assetTracker = Game::GetInstance()->GetAssetTracker();
		if (!myModelRes.empty())
		{
			myVisualObject->SetModel(assetTracker.GetOrCreate<Model>(myModelRes));
		}
		if (!myPipelineRes.empty())
		{
			myVisualObject->SetPipeline(assetTracker.GetOrCreate<Pipeline>(myPipelineRes));
		}
		if (!myTextureResources.empty())
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

	ASSERT_STR(myOwner->GetVisualObject() == nullptr, "Multi VisualObject not supported yet!");

	myVisualObject = new VisualObject(*myOwner);
	myOwner->SetVisualObject(myVisualObject);
}