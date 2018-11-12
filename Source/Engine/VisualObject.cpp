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
	if (myModel->GetState() == Resource::State::Invalid)
	{
		// it hasn't finished loading from disk yet, so have to wait
		myModel->SetOnLoadCB(bind(&VisualObject::UpdateCenter, this, std::placeholders::_1));
	}
	else
	{
		myGameObject.GetTransform().SetCenter(aModel->GetCenter());
	}
}

void VisualObject::SetPipeline(Handle<Pipeline> aPipeline)
{
	myPipeline = aPipeline;
	// Since we got a new pipeline, time to replace
	// descriptors, UBOs and adapters
	myUniforms.clear();
	myAdapters.clear();

	if (myPipeline.IsValid())
	{
		const Pipeline* pipeline = myPipeline.Get();
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