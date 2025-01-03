#pragma once

#include <Core/UID.h>
#include <Core/Debug/DebugDrawer.h>
#include <Core/Utils.h>
#include <Core/StableVector.h>

#include "GameTaskManager.h"
#include "GameObject.h"
#include "EngineSettings.h"
#include "UIWidgets/TopBar.h"
#include "World.h"

class Camera;
class Graphics;
class Terrain;
class PhysicsShapeBase;
class PhysicsComponent;
class ImGUISystem;
class AnimationSystem;
class RenderThread;
class AssetTracker;
struct GLFWwindow;
class Transform;
class LightSystem;
class FileWatcher;

class Game
{
public:
	struct Tasks
	{
		enum Type : uint16_t
		{
			BeginFrame = (uint16_t)(GameTask::ReservedTypes::GraphBroadcast)+1,
			AddGameObjects,
			PhysicsUpdate,
			AnimationUpdate,
			GameUpdate,
			RemoveGameObjects,
			UpdateEnd,
			Render,
			UpdateAudio,
			Last = UpdateAudio
		};
	};

	typedef void (*ReportError)(int, const char*);

	using DebugDrawerCallback = std::function<void(const DebugDrawer&)>;
	struct TerrainEntity
	{
		Terrain* myTerrain; // owning
		VisualObject* myVisualObject; // owning
		PhysicsComponent* myPhysComponent; // owning
	};

public:
	Game(ReportError aReporterFunc);
	~Game();

	// TODO: get rid of this
	static Game* GetInstance() { return ourInstance; }

	AssetTracker& GetAssetTracker();
	DebugDrawer& GetDebugDrawer() { return myDebugDrawer; }
	ImGUISystem& GetImGUISystem();
	AnimationSystem& GetAnimationSystem();
	LightSystem& GetLightSystem();
	GameTaskManager& GetTaskManager() { return *myTaskManager.get(); }

	void Init(bool aUseDefaultFinalCompositePass);
	void Run();
	void CleanUp();

	float GetLastFrameDeltaTime() const { return myDeltaTime; }
	bool IsRunning() const;
	void EndGame() { myShouldEnd = true; }
	bool IsPaused() const { return mySettings.myIsPaused; }
	// Returns whether the window is in focus thread safe way
	bool IsWindowInFocus() const { return myIsInFocus; }

	// Returns the pointer to the window. Querying things from the window
	// might not be threadsafe!
	GLFWwindow* GetWindow() const;

	World& GetWorld() { return myWorld; }
	const World& GetWorld() const { return myWorld; }
	template<class TFunc>
	void AccessRenderables(TFunc&& aFunc);
	template<class TFunc>
	void AccessTerrains(TFunc&& aFunc);
	template<class TFunc>
	void AccessTerrains(TFunc&& aFunc) const;
	void ForEachDebugDrawer(const DebugDrawerCallback& aFunc);

	Camera* GetCamera() const { return myCamera; }

	// utility method for accessing the time across game
	float GetTime() const;

	Graphics* GetGraphics();
	const Graphics* GetGraphics() const;

	EngineSettings& GetEngineSettings() { return mySettings; }
	const EngineSettings& GetEngineSettings() const { return mySettings; }

	// Adds GameObject and it's children to the world
	// Does not add the parent of the GameObject to the world -
	// it is assumed it's already been added
	void AddGameObject(Handle<GameObject> aGOHandle);
	// Removes GameObject and it's children from the world
	// If GameObject has a parent, it's retained in the world
	void RemoveGameObject(Handle<GameObject> aGOHandle);

	size_t GetRenderableCount() const { return myRenderables.GetCount(); }

	Renderable& CreateRenderable(GameObject& aGO);
	void DeleteRenderable(Renderable& aRenderable);

	void AddTerrain(Terrain* aTerrain /* owning */, Handle<Pipeline> aPipeline);
	void RemoveTerrain(size_t anIndex);
	// TODO: remove this (we have AccessTerrains), or find protect it with a mutex
	Terrain* GetTerrain(size_t anIndex) { return myTerrains[anIndex].myTerrain; }
	const Terrain* GetTerrain(size_t anIndex) const { return myTerrains[anIndex].myTerrain; }
	size_t GetTerrainCount() const { return myTerrains.size(); }

	// TODO: need to hide it and gate it around #ifdef _DEBUG
	static bool ourGODeleteEnabled;

private:
	void RunMainThread();

	void AddGameObjects();
	void BeginFrame();
	void Update();
	void PhysicsUpdate();
	void AnimationUpdate();
	void Render();
	void UpdateAudio();
	void UpdateEnd();
	void RemoveGameObjects();

	static Game* ourInstance;
	RenderThread* myRenderThread;
	std::unique_ptr<GameTaskManager> myTaskManager;

	// timer measurements
	float myFrameStart;
	float myDeltaTime;

	Utils::AffinitySetter myAffinitySetter{ Utils::AffinitySetter::Priority::Medium };
	Camera* myCamera;
	tbb::spin_mutex myAddLock, myRemoveLock;
	World myWorld;
	std::queue<Handle<GameObject>> myAddQueue;
	std::queue<Handle<GameObject>> myRemoveQueue;
	
	StableVector<Renderable> myRenderables;
	std::mutex myRenderablesMutex;
	
	std::vector<TerrainEntity> myTerrains;
	AssetTracker* myAssetTracker;
	// TODO: explore thread-local drawers!
	DebugDrawer myDebugDrawer;
	ImGUISystem* myImGUISystem;
	AnimationSystem* myAnimationSystem;
	LightSystem* myLightSystem;
	EngineSettings mySettings;
	TopBar myTopBar;
	FileWatcher* myFileWatcher;

	bool myIsRunning;
	bool myShouldEnd;
	bool myIsInFocus;
#ifdef ASSERT_MUTEX
	mutable AssertMutex myGOMutex;
	mutable AssertMutex myTerrainsMutex;
#endif
};

template<class TFunc>
void Game::AccessRenderables(TFunc&& aFunc)
{
	// TODO: replace with a read-only lock
	std::lock_guard lock(myRenderablesMutex);
	aFunc(myRenderables);
}

template<class TFunc>
void Game::AccessTerrains(TFunc&& aFunc)
{
#ifdef ASSERT_MUTEX
	AssertLock assertLock(myTerrainsMutex);
#endif
	aFunc(myTerrains);
}

template<class TFunc>
void Game::AccessTerrains(TFunc&& aFunc) const
{
#ifdef ASSERT_MUTEX
	AssertLock assertLock(myTerrainsMutex);
#endif
	aFunc(myTerrains);
}