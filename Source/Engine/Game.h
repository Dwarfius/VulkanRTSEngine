#pragma once

#include <Core/UID.h>
#include <Core/Debug/DebugDrawer.h>

#include "GameTaskManager.h"

class Camera;
class EditorMode;
class GameObject;
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

class Game
{
public:
	typedef void (*ReportError)(int, const char*);

	Game(ReportError aReporterFunc);
	~Game();

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

	const std::unordered_map<UID, GameObject*>& GetGameObjects() const { return myGameObjects; }
	size_t GetGameObjectCount() const { return myGameObjects.size(); }
	GameObject* Instantiate(glm::vec3 aPos = glm::vec3(), glm::vec3 aRot = glm::vec3(), glm::vec3 aScale = glm::vec3(1));
	// TODO: get rid of this limit by improving VK renderer
	constexpr static uint32_t kMaxObjects = 4000;

	Camera* GetCamera() const { return myCamera; }
	const Terrain* GetTerrain(glm::vec3 pos) const;
	PhysicsWorld* GetPhysicsWorld() const { return myPhysWorld; }

	// utility method for accessing the time across game
	float GetTime() const;

	Graphics* GetGraphics();
	const Graphics* GetGraphics() const;

	void RemoveGameObject(GameObject* aGo);

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

	void TestPool();

	static Game* ourInstance;
	RenderThread* myRenderThread;
	std::unique_ptr<GameTaskManager> myTaskManager;

	// timer measurements
	float myFrameStart;
	float myDeltaTime;

	EditorMode* myEditorMode;
	Camera* myCamera;
	tbb::spin_mutex myAddLock, myRemoveLock;
	std::unordered_map<UID, GameObject*> myGameObjects;
	std::queue<GameObject*> myAddQueue;
	std::queue<GameObject*> myRemoveQueue;
	std::vector<Terrain*> myTerrains;
	PhysicsWorld* myPhysWorld;
	AssetTracker* myAssetTracker;
	// TODO: explore thread-local drawers!
	DebugDrawer myDebugDrawer;
	ImGUISystem* myImGUISystem;
	AnimationSystem* myAnimationSystem;

	bool myIsRunning;
	bool myShouldEnd;
	bool myIsPaused;
	bool myIsInFocus;
};