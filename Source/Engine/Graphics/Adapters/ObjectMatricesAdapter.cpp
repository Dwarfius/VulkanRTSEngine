#include "Precomp.h"
#include "ObjectMatricesAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>

#include "../../VisualObject.h"

ObjectMatricesAdapter::ObjectMatricesAdapter(const GameObject& aGO, const VisualObject& aVO)
	: UniformAdapter(aGO, aVO)
{
}

void ObjectMatricesAdapter::FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const
{
	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already
	const glm::mat4& model = myVO.GetTransform().GetMatrix();
	const glm::mat4& view = aCam.GetView();
	const glm::mat4 modelView = view * model;
	const glm::mat4& vp = aCam.Get();

	aUB.SetUniform(0, model);
	aUB.SetUniform(1, modelView);
	aUB.SetUniform(2, vp * model);
}