#pragma once

#include <Core/RWBuffer.h>
#include <Graphics/Graphics.h>

class VisualObject;
class DebugDrawer;

// TODO: need to investigate and properly implement double/tripple buffering
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

	void AddRenderable(VisualObject* aVO);
	// TODO: this is not threadsafe!
	void AddDebugRenderable(const DebugDrawer* aDebugDrawer);

	void SubmitRenderables();

private:
	std::unique_ptr<Graphics> myGraphics;
	bool myIsUsingVulkan;

	std::queue<VisualObject*> myResolveQueue;
	RWBuffer<std::vector<const VisualObject*>, 2> myRenderables;
	RWBuffer<std::vector<const DebugDrawer*>, 2> myDebugDrawers;

	std::atomic<bool> myNeedsSwitch;
	std::atomic<bool> myHasWorkPending;
};

