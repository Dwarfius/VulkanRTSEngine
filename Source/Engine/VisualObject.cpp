#include "Precomp.h"
#include "VisualObject.h"

#include "Game.h"
#include "GameObject.h"
#include "Graphics/Adapters/UniformAdapter.h"
#include "Graphics/Adapters/UniformAdapterRegister.h"

#include <Graphics/GPUResource.h>
#include <Graphics/UniformBlock.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/GPUModel.h>

VisualObject::VisualObject(GameObject& aGO)
	: myGameObject(aGO)
	, myCategory(Category::GameObject)
{
}

void VisualObject::SetModel(Handle<Model> aModel)
{
	myModel = Game::GetInstance()->GetGraphics()->GetOrCreate(aModel);
}

void VisualObject::SetPipeline(Handle<Pipeline> aPipeline)
{
	myPipeline = Game::GetInstance()->GetGraphics()->GetOrCreate(aPipeline);

	if (myPipeline.IsValid())
	{
		Pipeline* pipeline = aPipeline.Get();
		if (pipeline->GetState() != Resource::State::Ready)
		{
			// pipeline hasn't finished loading, so there are no descriptors atm
			pipeline->AddOnLoadCB([=](const Resource* aRes) { UpdateDescriptors(aRes); });
		}
		else
		{
			UpdateDescriptors(pipeline);
		}
	}
}

void VisualObject::SetTexture(Handle<Texture> aTexture)
{
	myTexture = Game::GetInstance()->GetGraphics()->GetOrCreate(aTexture);
}

bool VisualObject::IsValid() const
{
	return myPipeline->GetState() == GPUResource::State::Valid
		&& myModel->GetState() == GPUResource::State::Valid
		&& myTexture->GetState() == GPUResource::State::Valid;
}

glm::vec3 VisualObject::GetCenter() const
{
	return myModel.Get<const GPUModel>()->GetCenter();
}

float VisualObject::GetRadius() const
{
	const glm::vec3 scale = myTransf.GetScale();
	const float maxScale = std::max({ scale.x, scale.y, scale.z });
	const float radius = myModel.Get<const GPUModel>()->GetSphereRadius();
	return maxScale * radius;
}

void VisualObject::UpdateDescriptors(const Resource* aPipelineRes)
{
	// Since we got a new pipeline, time to replace
	// descriptors, UBOs and adapters
	myUniforms.clear();
	myAdapters.clear();

	const Pipeline* pipeline = static_cast<const Pipeline*>(aPipelineRes);
	size_t descriptorCount = pipeline->GetDescriptorCount();
	for (size_t i = 0; i < descriptorCount; i++)
	{
		const Descriptor& descriptor = pipeline->GetDescriptor(i);

		myUniforms.push_back(std::make_shared<UniformBlock>(descriptor));
		const std::string& adapterName = descriptor.GetUniformAdapter();
		myAdapters.push_back(
			UniformAdapterRegister::GetInstance()->GetAdapter(adapterName, myGameObject, *this)
		);
	}
}