#include "Precomp.h"
#include "VisualObject.h"

#include "Graphics/UniformBlock.h"

VisualObject::VisualObject(Handle<Model> aModel, Handle<Pipeline> aPipeline, Handle<Texture> aTextureId)
	: myModel(aModel)
	, myPipeline(aPipeline)
	, myTexture(aTextureId)
{
	myUniforms = make_shared<UniformBlock>(myPipeline->GetDescriptor());
}

void VisualObject::SetPipeline(Handle<Pipeline> aPipeline)
{
	myPipeline = aPipeline;
	myUniforms = make_shared<UniformBlock>(myPipeline->GetDescriptor());
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