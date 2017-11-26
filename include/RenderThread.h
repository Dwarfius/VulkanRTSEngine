#pragma once

#include "Terrain.h"

class Graphics;

// a proxy class that handles the render thread
class RenderThread
{
public:
	RenderThread();
	~RenderThread();

	void Init(bool useVulkan, const vector<Terrain>* aTerrainSet);
	void Work();
	bool IsBusy() const { return workPending; }
	bool HasInitialised() const { return initialised; }

	Graphics* GetGraphicsRaw() { return graphics.get(); }
	const Graphics* GetGraphics() const { return graphics.get(); }

private:
	const vector<Terrain>* terrains;
	unique_ptr<Graphics> graphics;
	bool usingVulkan;

	unique_ptr<thread> renderThread;
	atomic<bool> initialised;
	atomic<bool> needsSwitch;
	atomic<bool> workPending;
	void InternalLoop();
};

