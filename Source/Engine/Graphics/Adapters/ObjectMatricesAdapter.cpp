#include "Precomp.h"
#include "ObjectMatricesAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>

#include "../../VisualObject.h"
#include "AdapterSourceData.h"

void ObjectMatricesAdapter::FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const
{
	const UniformAdapterSource& source = static_cast<const UniformAdapterSource&>(aData);

	const VisualObject& visualObject = source.myVO;
	const Camera& camera = source.myCam;

	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already
	const glm::mat4& model = visualObject.GetTransform().GetMatrix();
	const glm::mat4& view = camera.GetView();
	const glm::mat4 modelView = view * model;
	const glm::mat4& vp = camera.Get();

	aUB.SetUniform(0, 0, model);
	aUB.SetUniform(1, 0, modelView);
	aUB.SetUniform(2, 0, vp * model);
}