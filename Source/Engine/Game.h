#pragma once

#include <Core/UID.h>
#include <Core/Debug/DebugDrawer.h>

#include "GameTaskManager.h"
#include "GameObject.h"

class Camera;
class EditorMode;
class Graphics;
class Terrain;
class PhysicsWorld;
class PhysicsEntity;
class PhysicsShapeBase;
class PhysicsComponent;
class ImGUISystem;
class AnimationSystem;
class RenderThread;
class AssetTracker;
struct GLFWwindow;
class AnimationTest;
class Transform;

class Game
{
public:
	typedef void (*ReportError)(int, const char*);

	Game(ReportError aReporterFunc);
	~Game();

	// TODO: get rid of this
	static Game* GetInstance() { return ourInstance; }

	AssetTracker& GetAssetTracker();
	DebugDrawer& GetDebugDrawer() { return myDebugDrawer; }
	ImGUISystem& GetImGUISystem();
	AnimationSystem& GetAnimationSystem();

	void Init();
	void RunMainThread();
	void CleanUp();

	bool IsRunning() const;
	void EndGame() { myShouldEnd = true; }
	bool IsPaused() const { return myIsPaused; }
	// Returns whether the window is in focus thread safe way
	bool IsWindowInFocus() const { return myIsInFocus; }
	// Returns the pointer to the window. Querying things from the window
	// might not be threadsafe!
	GLFWwindow* GetWindow() const;

	size_t GetGameObjectCount() const { return myGameObjects.size(); }
	template<class TFunc>
	void ForEach(TFunc aFunc);

	Camera* GetCamera() const { return myCamera; }
	const Terrain* GetTerrain(glm::vec3 pos) const;
	PhysicsWorld* GetPhysicsWorld() const { return myPhysWorld; }

	// utility method for accessing the time across game
	float GetTime() const;

	Graphics* GetGraphics();
	const Graphics* GetGraphics() const;

	// Adds GameObject and it's children to the world
	// Does not add the parent of the GameObject to the world -
	// it is assumed it's already been added
	void AddGameObject(Handle<GameObject> aGOHandle);
	// Removes GameObject and it's children from the world
	// If GameObject has a parent, it's retained in the world
	void RemoveGameObject(Handle<GameObject> aGOHandle);

	// TODO: need to hide it and gate it around #ifdef _DEBUG
	static bool ourGODeleteEnabled;

private:
	void AddGameObjects();
	void UpdateInput();
	void Update();
	void EditorUpdate();
	void PhysicsUpdate();
	void AnimationUpdate();
	void Render();
	void UpdateAudio();
	void UpdateEnd();
	void RemoveGameObjects();

	void RegisterUniformAdapters();

	static Game* ourInstance;
	RenderThread* myRenderThread;
	std::unique_ptr<GameTaskManager> myTaskManager;

	// timer measurements
	float myFrameStart;
	float myDeltaTime;

	EditorMode* myEditorMode;
	Camera* myCamera;
	tbb::spin_mutex myAddLock, myRemoveLock;
	std::unordered_map<UID, Handle<GameObject>> myGameObjects;
	std::queue<Handle<GameObject>> myAddQueue;
	std::queue<Handle<GameObject>> myRemoveQueue;
	std::vector<Terrain*> myTerrains;
	PhysicsWorld* myPhysWorld;
	AssetTracker* myAssetTracker;
	// TODO: explore thread-local drawers!
	DebugDrawer myDebugDrawer;
	ImGUISystem* myImGUISystem;
	AnimationSystem* myAnimationSystem;
	AnimationTest* myAnimTest;

	bool myIsRunning;
	bool myShouldEnd;
	bool myIsPaused;
	bool myIsInFocus;
	mutable AssertRWMutex myGOMutex;
};

template<class TFunc>
void Game::ForEach(TFunc aFunc)
{
	AssertReadLock assertLock(myGOMutex);
	for (auto& [id, go] : myGameObjects)
	{
		aFunc(*go.Get());
	}
}