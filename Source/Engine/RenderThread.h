#pragma once

#include <Graphics/Graphics.h>

class RenderThread
{
public:
	RenderThread();
	~RenderThread();

	void Init(bool anUseVulkan, AssetTracker& anAssetTracker);
	void Gather();
	bool HasWork() const { return myHasWorkPending; }
	void RequestSwitch() { myNeedsSwitch = true; }
	GLFWwindow* GetWindow() const;

	Graphics* GetGraphics() { return myGraphics.get(); }
	const Graphics* GetGraphics() const { return myGraphics.get(); }

	void SubmitRenderables();

private:
	std::unique_ptr<Graphics> myGraphics;

	std::atomic<bool> myNeedsSwitch;
	std::atomic<bool> myHasWorkPending;
	bool myIsUsingVulkan;
};