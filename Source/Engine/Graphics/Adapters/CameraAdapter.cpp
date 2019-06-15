#include "Precomp.h"
#include "CameraAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>

#include "../../VisualObject.h"

CameraAdapter::CameraAdapter(const GameObject& aGO, const VisualObject& aVO)
	: UniformAdapter(aGO, aVO)
{
}

void CameraAdapter::FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const
{
	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already
	glm::vec3 pos = aCam.GetTransform().GetPos();
	glm::vec3 right = aCam.GetTransform().GetRight();
	glm::vec3 up = aCam.GetTransform().GetUp();
	glm::vec3 forward = aCam.GetTransform().GetForward();
	glm::mat4 viewMatrix = aCam.GetView();
	glm::mat4 projMatrix = aCam.GetProj();
	const Frustum& frustum = aCam.GetFrustum();
	glm::vec4 camPack1(frustum.myNearPlane, frustum.myFarPlane, 0, 0);
	glm::vec4 camPack2(frustum.mySphereFactorX, frustum.mySphereFactorY, frustum.myRatio, frustum.myTangent);

	aUB.SetUniform(0, pos);
	aUB.SetUniform(1, right);
	aUB.SetUniform(2, up);
	aUB.SetUniform(3, forward);
	aUB.SetUniform(4, viewMatrix);
	aUB.SetUniform(5, projMatrix);
	aUB.SetUniform(6, camPack1);
	aUB.SetUniform(7, camPack2);
}