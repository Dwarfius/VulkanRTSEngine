#include "Precomp.h"
#include "CameraAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>
#include <Graphics/Graphics.h>

void CameraAdapter::FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const
{
	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already
	const Camera& camera = aData.myCam;
	const glm::vec3 pos = camera.GetTransform().GetPos();
	const glm::mat4 viewMatrix = camera.GetView();
	const glm::mat4 projMatrix = camera.GetProj();
	const Frustum& frustum = camera.GetFrustum();
	const glm::vec2 viewport(Graphics::GetWidth(), Graphics::GetHeight());

	aUB.SetUniform(0, viewMatrix);
	aUB.SetUniform(1, projMatrix);
	aUB.SetUniform(2, frustum.myPlanes[0]);
	aUB.SetUniform(3, frustum.myPlanes[1]);
	aUB.SetUniform(4, frustum.myPlanes[2]);
	aUB.SetUniform(5, frustum.myPlanes[3]);
	aUB.SetUniform(6, frustum.myPlanes[4]);
	aUB.SetUniform(7, frustum.myPlanes[5]);
	aUB.SetUniform(8, pos);
	aUB.SetUniform(9, viewport);
}