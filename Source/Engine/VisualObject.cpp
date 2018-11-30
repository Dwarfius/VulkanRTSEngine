#include "Precomp.h"
#include "VisualObject.h"
#include "GameObject.h"

#include "Graphics/UniformAdapter.h"
#include "Graphics/UniformBlock.h"
#include "Graphics/UniformAdapterRegister.h"

VisualObject::VisualObject(GameObject& aGO)
	: myGameObject(aGO)
{
}

void VisualObject::SetModel(Handle<Model> aModel)
{
	myModel = aModel;

	if (myModel.IsValid())
	{
		Model* model = myModel.Get();
		if (model->GetState() == Resource::State::Invalid)
		{
			// it hasn't finished loading from disk yet, so have to wait
			model->AddOnLoadCB(bind(&VisualObject::UpdateCenter, this, std::placeholders::_1));
		}
		else
		{
			UpdateCenter(model);
		}
	}
}

void VisualObject::SetPipeline(Handle<Pipeline> aPipeline)
{
	myPipeline = aPipeline;

	if (myPipeline.IsValid())
	{
		Pipeline* pipeline = myPipeline.Get();
		if (pipeline->GetState() == Resource::State::Invalid)
		{
			// pipeline hasn't finished loading, so there are no descriptors atm
			pipeline->AddOnLoadCB(bind(&VisualObject::UpdateDescriptors, this, std::placeholders::_1));
		}
		else
		{
			UpdateDescriptors(pipeline);
		}
	}
}

bool VisualObject::IsValid() const
{
	return myPipeline->GetState() == Resource::State::Ready
		&& myModel->GetState() == Resource::State::Ready
		&& myTexture->GetState() == Resource::State::Ready;
}

glm::vec3 VisualObject::GetCenter() const
{
	return myModel->GetCenter();
}

float VisualObject::GetRadius() const
{
	const glm::vec3 scale = myTransf.GetScale();
	const float maxScale = max({ scale.x, scale.y, scale.z });
	const float radius = myModel->GetSphereRadius();
	return maxScale * radius;
}

void VisualObject::UpdateCenter(const Resource* aModelRes)
{
	const Model* model = static_cast<const Model*>(aModelRes);
	myGameObject.GetTransform().SetCenter(model->GetCenter());
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

		myUniforms.push_back(make_shared<UniformBlock>(descriptor));
		const string& adapterName = descriptor.GetUniformAdapter();
		myAdapters.push_back(
			UniformAdapterRegister::GetInstance()->GetAdapter(adapterName, myGameObject, *this)
		);
	}
}