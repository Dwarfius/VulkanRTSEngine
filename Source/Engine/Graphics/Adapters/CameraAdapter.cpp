#include "Precomp.h"
#include "CameraAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>
#include <Graphics/Graphics.h>

void CameraAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already
	const CameraAdapterSourceData& data = static_cast<const CameraAdapterSourceData&>(aData);
	const Camera& camera = data.myCam;
	const glm::vec3 pos = camera.GetTransform().GetPos();
	const glm::mat4 viewProj = camera.Get();
	const glm::mat4 viewMatrix = camera.GetView();
	const glm::mat4 projMatrix = camera.GetProj();
	const Frustum& frustum = camera.GetFrustum();
	const glm::vec2 viewport(data.myGraphics.GetWidth(), data.myGraphics.GetHeight());

	aUB.SetUniform(0, 0, viewProj);
	aUB.SetUniform(1, 0, viewMatrix);
	aUB.SetUniform(2, 0, projMatrix);
	for (uint8_t i = 0; i < 6; i++)
	{
		aUB.SetUniform(3, i, frustum.myPlanes[i]);
	}
	aUB.SetUniform(4, 0, pos);
	aUB.SetUniform(5, 0, viewport);
}