#pragma once

#include <Core/RWBuffer.h>
#include <Graphics/Graphics.h>

class VisualObject;
class DebugDrawer;
class Terrain;
class GameObject;
class Camera;

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

	void AddRenderable(const VisualObject& aVO, const GameObject& aGO);
	void AddTerrainRenderable(const VisualObject& aVO, const Terrain& aTerrain);
	void AddDebugRenderable(const DebugDrawer& aDebugDrawer);

	void SubmitRenderables();

private:
	std::unique_ptr<Graphics> myGraphics;

	struct GameObjectRenderable
	{
		const VisualObject& myVisualObject;
		const GameObject& myGameObject;
	};
	RWBuffer<std::vector<GameObjectRenderable>, 2> myRenderables;
	void ScheduleGORenderables(const Camera& aCam);

	struct TerrainRenderable
	{
		const VisualObject& myVisualObject;
		const Terrain& myTerrain;
	};
	RWBuffer<std::vector<TerrainRenderable>, 2> myTerrainRenderables;
	void ScheduleTerrainRenderables(const Camera& aCam);

	struct DebugRenderable
	{
		const DebugDrawer& myDebugDrawer;
	};
	RWBuffer<std::vector<DebugRenderable>, 2> myDebugDrawers;
	void ScheduleDebugRenderables(const Camera& aCam);

	std::atomic<bool> myNeedsSwitch;
	std::atomic<bool> myHasWorkPending;
	bool myIsUsingVulkan;
};

