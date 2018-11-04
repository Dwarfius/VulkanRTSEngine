#include "Precomp.h"
#include "VisualObject.h"

#include "Graphics/UniformAdapter.h"
#include "Graphics/UniformBlock.h"

VisualObject::VisualObject(const GameObject& aGO)
	: myGameObject(aGO)
{
}

void VisualObject::SetPipeline(Handle<Pipeline> aPipeline)
{
	myPipeline = aPipeline;
	const Descriptor& descriptor = myPipeline->GetDescriptor();
	myUniforms = make_shared<UniformBlock>(descriptor);
	// HACK: need to implement a proper adapter handler
	ASSERT_STR(descriptor.GetUniformAdapter() == "default", "Get rid of the hack!");
	myAdapter = make_shared<UniformAdapter>(myGameObject, *this);
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