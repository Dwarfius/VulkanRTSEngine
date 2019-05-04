#pragma once

#include <LinearMath/btIDebugDraw.h>
#include <Core/Vertex.h>
#include <Core/Debug/DebugDrawer.h>

// TODO: move this to private sources section
// TODO: instead of caching all lines, it should work with a generic debug util
class PhysicsDebugDrawer : public btIDebugDraw
{
public:
	PhysicsDebugDrawer();

	const DebugDrawer& GetDebugDrawer() const { return myDebugDrawer; }
	
	// btIDebugDraw interface
	void drawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aColor) override final;
	void drawLine(const btVector3& aFrom, const btVector3& aTo, const btVector3& aFromColor, const btVector3& aToColor) override final;
	void drawContactPoint(const btVector3& aPointOnB, const btVector3& aNormalOnB, btScalar aDistance, int aLifeTime, const btVector3& aColor) override;
	void reportErrorWarning(const char* aWarningString) override {}
	void draw3dText(const btVector3& aLocation, const char* aTextString) override {}
	
	void setDebugMode(int aDebugMode) override { myDebugMode = aDebugMode; }
	int	getDebugMode() const override { return myDebugMode; }

	void clearLines() override;
	//

private:
	DebugDrawer myDebugDrawer;
	int myDebugMode;
};