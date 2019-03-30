#pragma once

#include <Core/RWBuffer.h>
#include <Core/Graphics/Graphics.h>

class VisualObject;

// a proxy class that handles the render thread
class RenderThread
{
public:
	RenderThread();
	~RenderThread();

	void Init(bool anUseVulkan, AssetTracker& anAssetTracker);
	void Work();
	bool HasWork() const { return myHasWorkPending; }
	void RequestSwitch() { myNeedsSwitch = true; }
	GLFWwindow* GetWindow() const;

	Graphics* GetGraphics() { return myGraphics.get(); }
	const Graphics* GetGraphics() const { return myGraphics.get(); }

	void AddRenderable(const VisualObject* aVO);
	void AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor);
	void AddLines(const vector<PosColorVertex>& aLineCache);

	void SubmitRenderables();

private:
	unique_ptr<Graphics> myGraphics;
	bool myIsUsingVulkan;

	// TODO: separate gameobject and renderable
	RWBuffer<vector<const VisualObject*>, 2> myTrippleRenderables;
	RWBuffer<vector<PosColorVertex>, 2> myTrippleLines;

	atomic<bool> myNeedsSwitch;
	atomic<bool> myHasWorkPending;
};

