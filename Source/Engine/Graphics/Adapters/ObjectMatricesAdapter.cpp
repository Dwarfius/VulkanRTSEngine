#include "Precomp.h"
#include "ObjectMatricesAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>

#include "../../VisualObject.h"
#include "AdapterSourceData.h"

void ObjectMatricesAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const UniformAdapterSource& source = static_cast<const UniformAdapterSource&>(aData);

	const Camera& camera = source.myCam;

	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already
	const glm::mat4& model = source.myVO.GetTransform().GetMatrix();
	const glm::mat4& view = camera.GetView();
	const glm::mat4& vp = camera.Get();

	aUB.SetUniform(ourDescriptor.GetOffset(0, 0), model);
	aUB.SetUniform(ourDescriptor.GetOffset(1, 0), view * model);
	aUB.SetUniform(ourDescriptor.GetOffset(2, 0), vp * model);
}