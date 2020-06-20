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
#include <Graphics/Resources/GPUPipeline.h>

VisualObject::VisualObject(const GameObject& aGO)
	: myGameObject(aGO)
	, myCategory(Category::GameObject)
	, myIsResolved(false)
{
}

void VisualObject::SetModel(Handle<Model> aModel)
{
	myModel = Game::GetInstance()->GetGraphics()->GetOrCreate(aModel);
}

void VisualObject::SetPipeline(Handle<Pipeline> aPipeline)
{
	myPipeline = Game::GetInstance()->GetGraphics()->GetOrCreate(aPipeline);
}

void VisualObject::SetTexture(Handle<Texture> aTexture)
{
	myTexture = Game::GetInstance()->GetGraphics()->GetOrCreate(aTexture);
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

bool VisualObject::Resolve()
{
	if (myPipeline->GetState() != GPUResource::State::Valid)
	{
		return false;
	}

	// Since we got a new pipeline, time to replace
	// descriptors, UBOs and adapters
	myUniforms.clear();
	myAdapters.clear();

	const GPUPipeline* pipeline = myPipeline.Get<const GPUPipeline>();
	size_t descriptorCount = pipeline->GetDescriptorCount();
	for (size_t i = 0; i < descriptorCount; i++)
	{
		const Handle<Descriptor>& descriptor = pipeline->GetDescriptor(i);

		myUniforms.push_back(std::make_shared<UniformBlock>(*descriptor.Get()));
		const std::string& adapterName = descriptor->GetUniformAdapter();
		myAdapters.push_back(
			UniformAdapterRegister::GetInstance()->GetAdapter(adapterName, myGameObject, *this)
		);
	}

	myIsResolved = true;
	return myIsResolved;
}