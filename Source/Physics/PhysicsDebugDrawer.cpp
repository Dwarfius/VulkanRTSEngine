#include "Common.h"
#include "PhysicsDebugDrawer.h"

PhysicsDebugDrawer::PhysicsDebugDrawer()
	: myDebugMode(btIDebugDraw::DBG_NoDebug)
{
	myLineCache.reserve(4000);
}

void PhysicsDebugDrawer::drawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aColor)
{
	glm::vec3 from = Utils::ConvertToGLM(aFrom);
	glm::vec3 to = Utils::ConvertToGLM(aTo);
	glm::vec3 color = Utils::ConvertToGLM(aColor);
	myLineCache.push_back(LineDraw{ from, color, to, color });
}