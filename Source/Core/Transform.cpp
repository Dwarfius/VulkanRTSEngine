#include "Precomp.h"
#include "Transform.h"

#include "Resources/Serializer.h"

Transform::Transform()
	: Transform(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f))
{
}

Transform::Transform(glm::vec3 aPos, glm::quat aRot, glm::vec3 aScale)
	: myPos(aPos)
	, myScale(aScale)
	, myRotation(aRot)
{
}

void Transform::LookAt(glm::vec3 aTarget)
{
	const glm::vec3 forward = glm::normalize(aTarget - myPos);
	if (glm::length2(forward) < 0.0001f)
	{
		return;
	}

	if (glm::abs(forward.y) >= 0.99f)
	{
		myRotation = glm::quatLookAtLH(forward, { 1, 0, 0 });
	}
	else
	{
		myRotation = glm::quatLookAtLH(forward, { 0, 1, 0 });
	}
}

glm::mat4 Transform::GetMatrix() const
{
	glm::mat4 modelMatrix;
	modelMatrix = glm::translate(glm::mat4(1), myPos);
	modelMatrix = modelMatrix * glm::mat4_cast(myRotation);
	modelMatrix = glm::scale(modelMatrix, myScale);
	return modelMatrix;
}

Transform Transform::operator*(const Transform& aOther) const
{
	const glm::vec3 totalScale = myScale * aOther.myScale;
	const glm::quat totalRot = myRotation * aOther.myRotation;
	const glm::vec3 totalPos = myPos + (myRotation * aOther.myPos) 
									* myScale;
	return { totalPos, totalRot, totalScale };
}

Transform Transform::GetInverted() const
{
	const glm::vec3 invertedScale = 1.f / myScale;
	const glm::quat invertedRot = glm::inverse(myRotation);
	const glm::vec3 invertedPos = -myPos;
	return { invertedPos, invertedRot, invertedScale };
}

glm::vec3 Transform::RotateAround(glm::vec3 aPoint, glm::vec3 aRefPoint, glm::vec3 anAngles)
{
	aPoint -= aRefPoint;
	aPoint = glm::quat(anAngles) * aPoint;
	return aPoint + aRefPoint;
}

void Transform::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myPos", myPos);
	aSerializer.Serialize("myScale", myScale);
	aSerializer.Serialize("myRotation", myRotation);
}