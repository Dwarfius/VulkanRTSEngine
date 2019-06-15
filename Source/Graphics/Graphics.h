#pragma once

#include <Core/Vertex.h>
#include "Descriptor.h"
#include "UniformBlock.h"
#include "IGPUAllocator.h"
#include "RenderPass.h"

class VisualObject;
struct GLFWwindow;
class Camera;
class Terrain;
class AssetTracker;
class DebugDrawer;
class RenderPassJob;
class RenderContext;

class Graphics : public IGPUAllocator
{
public:
	Graphics(AssetTracker& anAssetTracker);

	virtual void Init() = 0;
	virtual void BeginGather();
	void Render(IRenderPass::Category aCategory, const Camera& aCam, const RenderJob& aJob, const IRenderPass::IParams& aParams);
	virtual void Display();
	virtual void CleanUp() = 0;
	
	virtual void PrepareLineCache(size_t aCacheSize) = 0;
	virtual void RenderDebug(const Camera& aCam, const DebugDrawer& aDebugDrawer) = 0;

	GLFWwindow* GetWindow() const { return myWindow; }
	
	uint32_t GetRenderCalls() const { return myRenderCalls; }

	static float GetWidth() { return static_cast<float>(ourWidth); }
	static float GetHeight() { return static_cast<float>(ourHeight); }

	// Graphics will manage the lifetime of the render pass job,
	// but it guarantees that it will stay valid until the next call
	// to GetRenderPassJob with same context.
	// Threadsafe
	[[nodiscard]] 
	virtual RenderPassJob& GetRenderPassJob(uint32_t anId, const RenderContext& renderContext) = 0;

	// Notifies the rendering system about how many threads will access it
	virtual void SetMaxThreads(uint32_t aMaxThreadCount) {}

	// Adds a render pass to graphics
	// takes ownership of the renderpass
	void AddRenderPass(IRenderPass* aRenderPass);

protected:
	GLFWwindow* myWindow;
	static int ourWidth, ourHeight;

	static Graphics* ourActiveGraphics;

	AssetTracker& myAssetTracker;

	uint32_t myRenderCalls;

	std::vector<IRenderPass*> myRenderPasses;
};