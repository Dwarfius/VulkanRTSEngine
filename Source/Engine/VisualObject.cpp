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
	const Descriptor& descriptor = myPipeline->GetDescriptor();
	myUniforms = make_shared<UniformBlock>(descriptor);
	const string& adapterName = descriptor.GetUniformAdapter();
	myAdapter = UniformAdapterRegister::GetInstance()->GetAdapter(adapterName, myGameObject, *this);
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