#include "Common.h"
#include "PhysicsDebugDrawer.h"

PhysicsDebugDrawer::PhysicsDebugDrawer()
	: myDebugMode(btIDebugDraw::DBG_NoDebug)
{
	myLineCache.reserve(4000);
	myLineLives.reserve(4000);
}

void PhysicsDebugDrawer::drawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aColor)
{
	DrawLine(aFrom, aTo, aColor, 0);
}

void PhysicsDebugDrawer::drawContactPoint(const btVector3& aPointOnB, const btVector3& aNormalOnB, btScalar aDistance, int aLifeTime, const btVector3& aColor)
{
	//drawSphere(aPointOnB, 0.1f, aColor);
	// HACK: aLifeTime is frames to live
	DrawLine(aPointOnB, aPointOnB + aNormalOnB * aDistance, aColor, aLifeTime);
}

void PhysicsDebugDrawer::clearLines()
{
	// TODO: refactor this away from here
	size_t size = myLineCache.size();
	if (size > 0)
	{
		size_t swapInd = size - 1;
		for (size_t i = 0; i <= swapInd && swapInd >= 1;)
		{
			if (--myLineLives[i] < 0.f)
			{
				swap(myLineCache[i], myLineCache[swapInd]);
				swap(myLineLives[i], myLineLives[swapInd]);
				swapInd--;
			}
			else
			{
				i++;
			}
		}
		// it would be perfect to just move the end head of the vector to the new position,
		// so that everything after the new end will just be junk data, but doesn't seem vec allows it
		// so have to call the erase which mustr trigger destructors
		// TODO: roll my own cache type with ability to move the start-end heads
		if (swapInd + 1 < size)
		{
			myLineCache.erase(myLineCache.begin() + swapInd + 1, myLineCache.end());
			myLineLives.erase(myLineLives.begin() + swapInd + 1, myLineLives.end());
		}
	}
}

void PhysicsDebugDrawer::DrawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aColor, int aLife)
{
	glm::vec3 from = Utils::ConvertToGLM(aFrom);
	glm::vec3 to = Utils::ConvertToGLM(aTo);
	glm::vec3 color = Utils::ConvertToGLM(aColor);
	myLineCache.push_back(LineDraw{ from, color, to, color });
	myLineLives.push_back(aLife);
}