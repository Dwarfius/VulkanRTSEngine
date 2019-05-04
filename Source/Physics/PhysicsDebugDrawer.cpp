#include "Precomp.h"
#include "PhysicsDebugDrawer.h"

PhysicsDebugDrawer::PhysicsDebugDrawer()
	: myDebugMode(btIDebugDraw::DBG_NoDebug)
{
}

void PhysicsDebugDrawer::drawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aColor)
{
	glm::vec3 from = Utils::ConvertToGLM(aFrom);
	glm::vec3 to = Utils::ConvertToGLM(aTo);
	glm::vec3 color = Utils::ConvertToGLM(aColor);
	myDebugDrawer.AddLine(from, to, color);
}

void PhysicsDebugDrawer::drawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aFromColor, const btVector3& aToColor)
{
	glm::vec3 from = Utils::ConvertToGLM(aFrom);
	glm::vec3 to = Utils::ConvertToGLM(aTo);
	glm::vec3 fromColor = Utils::ConvertToGLM(aFromColor);
	glm::vec3 toColor = Utils::ConvertToGLM(aToColor);
	myDebugDrawer.AddLine(from, to, fromColor, toColor);
}

void PhysicsDebugDrawer::drawContactPoint(const btVector3& aPointOnB, const btVector3& aNormalOnB, btScalar aDistance, int aLifeTime, const btVector3& aColor)
{
	// I don't know why there's an aLifeTime argument, since
	// Bullet internally keeps track how long it should be drawn for
	drawLine(aPointOnB, aPointOnB + aNormalOnB * aDistance, aColor);
}

void PhysicsDebugDrawer::clearLines()
{
	myDebugDrawer.BeginFrame();
}