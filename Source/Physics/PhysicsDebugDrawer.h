#pragma once

#include <LinearMath/btIDebugDraw.h>

// TODO: implement a command queue for phys world and entity commands
class PhysicsDebugDrawer : public btIDebugDraw
{
public:
	struct LineDraw
	{
		glm::vec3 myFrom;
		glm::vec3 myColorFrom;
		glm::vec3 myTo;
		glm::vec3 myColorTo;
	};

	PhysicsDebugDrawer();

	void drawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aColor) override;
	void drawContactPoint(const btVector3& aPointOnB, const btVector3& aNormalOnB, btScalar aDistance, int aLifeTime, const btVector3& aColor) {}
	void reportErrorWarning(const char* aWarningString) override {}
	void draw3dText(const btVector3& aLocation, const char* aTextString) override {}
	
	void setDebugMode(int aDebugMode) override { myDebugMode = aDebugMode; }
	int	getDebugMode() const override { return myDebugMode; }

	void clearLines() override { myLineCache.clear(); }

	const vector<LineDraw>& GetLineCache() const { return myLineCache; }

private:
	int myDebugMode;

	vector<LineDraw> myLineCache;
};