#include "Precomp.h"
#include "UniformAdapter.h"

#include <Core/Camera.h>
#include <Core/Graphics/UniformBlock.h>
#include "VisualObject.h"

UniformAdapter::UniformAdapter(const GameObject& aGO, const VisualObject& aVO)
	: myGO(aGO)
	, myVO(aVO)
{
}

void UniformAdapter::FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const
{
	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already
	const glm::mat4& model = myVO.GetTransform().GetMatrix();
	const glm::mat4& vp = aCam.Get();

	aUB.SetUniform(0, model);
	aUB.SetUniform(1, vp * model);
}