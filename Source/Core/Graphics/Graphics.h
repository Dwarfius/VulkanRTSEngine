#pragma once

#include <unordered_map>
#include <type_traits>

#include "../Vertex.h"
#include "Descriptor.h"
#include "UniformBlock.h"
#include "IGPUAllocator.h"

class VisualObject;
struct GLFWwindow;
class Camera;
class Terrain;
class AssetTracker;
class DebugDrawer;

class Graphics : public IGPUAllocator
{
public:
	Graphics(AssetTracker& anAssetTracker);

	virtual void Init() = 0;
	virtual void BeginGather() = 0;
	virtual void Render(const Camera& aCam, const VisualObject* aGO) = 0;
	virtual void Display() = 0;
	virtual void CleanUp() = 0;
	
	virtual void PrepareLineCache(size_t aCacheSize) = 0;
	virtual void RenderDebug(const Camera& aCam, const DebugDrawer& aDebugDrawer) = 0;

	GLFWwindow* GetWindow() const { return myWindow; }
	
	uint32_t GetRenderCalls() const { return myRenderCalls; }
	void ResetRenderCalls() { myRenderCalls = 0; }

	static float GetWidth() { return static_cast<float>(ourWidth); }
	static float GetHeight() { return static_cast<float>(ourHeight); }

	// Notifies the rendering system about how many threads will access it
	virtual void SetMaxThreads(uint32_t aMaxThreadCount) {}

protected:
	GLFWwindow* myWindow;
	static int ourWidth, ourHeight;

	static Graphics* ourActiveGraphics;

	AssetTracker& myAssetTracker;

	uint32_t myRenderCalls;
};