#pragma once

#include <LinearMath/btIDebugDraw.h>

// TODO: move this to private sources section
// TODO: instead of caching all lines, it should work with a generic debug util
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

	const vector<LineDraw>& GetLineCache() const { return myLineCache; }

	// btIDebugDraw interface
	void drawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aColor) override;
	void drawContactPoint(const btVector3& aPointOnB, const btVector3& aNormalOnB, btScalar aDistance, int aLifeTime, const btVector3& aColor) override;
	void reportErrorWarning(const char* aWarningString) override {}
	void draw3dText(const btVector3& aLocation, const char* aTextString) override {}
	
	void setDebugMode(int aDebugMode) override { myDebugMode = aDebugMode; }
	int	getDebugMode() const override { return myDebugMode; }

	void clearLines() override;
	//

private:
	void DrawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aColor, int aLife);

	int myDebugMode;

	vector<LineDraw> myLineCache;
	vector<int> myLineLives;
};