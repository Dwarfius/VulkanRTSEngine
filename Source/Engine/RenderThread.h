#pragma once

#include "RWBuffer.h"
#include "Graphics.h"

class GameObject;
class Terrain;

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
	void AddLines(const vector<Graphics::LineDraw>& aLineCache);

	void SubmitRenderables();

private:
	const vector<Terrain*>* myTerrains;
	unique_ptr<Graphics> myGraphics;
	bool myIsUsingVulkan;

	// TODO: separate gameobject and renderable
	RWBuffer<vector<const GameObject*>, 2> myTrippleRenderables;
	RWBuffer<vector<Graphics::LineDraw>, 2> myTrippleLines;

	atomic<bool> myNeedsSwitch;
	atomic<bool> myHasWorkPending;
};

