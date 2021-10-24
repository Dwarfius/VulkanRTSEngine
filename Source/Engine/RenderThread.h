#pragma once

#include <Graphics/Graphics.h>
#include <Core/Threading/AssertMutex.h>

class RenderThread
{
public:
	using OnRenderCallback = std::function<void(Graphics&)>;

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

	void AddRenderContributor(OnRenderCallback aCallback);
	void SubmitRenderables();

private:
	std::unique_ptr<Graphics> myGraphics;
	std::vector<OnRenderCallback> myRenderCallbacks;
#ifdef ASSERT_MUTEX
	AssertMutex myRenderCallbackMutex;
#endif

	std::atomic<bool> myNeedsSwitch;
	std::atomic<bool> myHasWorkPending;
	bool myIsUsingVulkan;
};