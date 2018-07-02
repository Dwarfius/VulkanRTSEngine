#pragma once

#include "TrippleBuffer.h"
#include "PhysicsDebugDrawer.h"

class Graphics;
class GameObject;
class Terrain;
struct GLFWwindow;

// a proxy class that handles the render thread
class RenderThread
{
public:
	RenderThread();
	~RenderThread();

	void Init(bool anUseVulkan, const vector<Terrain*>* aTerrainSet);
	void Work();
	bool HasWork() const { return myHasWorkPending; }
	void RequestSwitch() { myNeedsSwitch = true; }
	GLFWwindow* GetWindow() const;

	Graphics* GetGraphics() { return myGraphics.get(); }
	const Graphics* GetGraphics() const { return myGraphics.get(); }

	void AddRenderable(const GameObject* aGo);
	void AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor);
	void AddLines(const vector<PhysicsDebugDrawer::LineDraw>& aLineCache);

	void SubmitRenderables();

private:
	const vector<Terrain*>* myTerrains;
	unique_ptr<Graphics> myGraphics;
	bool myIsUsingVulkan;

	// TODO: separate gameobject and renderable
	TrippleBuffer<vector<const GameObject*>> myTrippleRenderables;
	TrippleBuffer<vector<PhysicsDebugDrawer::LineDraw>> myTrippleLines;

	atomic<bool> myNeedsSwitch;
	atomic<bool> myHasWorkPending;
};

