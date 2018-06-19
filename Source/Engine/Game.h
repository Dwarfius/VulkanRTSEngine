#pragma once

#include "GameTaskManager.h"
#include "RenderThread.h"
#include "UID.h"

class GameObject;
class Camera;
class Graphics;
class Terrain;
class PhysicsWorld;

class Game
{
public:
	typedef void (*ReportError)(int, const char*);

	Game(ReportError aReporterFunc);
	~Game();

	static Game* GetInstance() { return ourInstance; }

	void Init();
	void RunMainThread();
	void RunTaskGraph();
	void CleanUp();

	bool IsRunning() const;
	void EndGame() { myShouldEnd = true; }
	bool IsPaused() const { return myIsPaused; }
	GLFWwindow* GetWindow() const;

	const unordered_map<UID, GameObject*>& GetGameObjects() const { return myGameObjects; }
	size_t GetGameObjectCount() const { return myGameObjects.size(); }
	GameObject* Instantiate(glm::vec3 pos = glm::vec3(), glm::vec3 rot = glm::vec3(), glm::vec3 scale = glm::vec3(1));
	const static uint32_t maxObjects = 4000;

	Camera* GetCamera() const { return myCamera; }
	const Terrain* GetTerrain(glm::vec3 pos) const;

	// utility method for accessing the time across game
	float GetTime() const;

	Graphics* GetGraphics() { return myRenderThread->GetGraphics(); }
	// TODO: figure out a way to force render-thread to return const RenderThread*
	const Graphics* GetGraphics() const { return myRenderThread->GetGraphics(); }

	void RemoveGameObject(GameObject* aGo);

	// TODO: need to hide it and gate it around #ifdef _DEBUG
	static bool ourGODeleteEnabled;

private:
	void AddGameObjects();
	void UpdateInput();
	void Update();
	void PhysicsUpdate();
	void Render();
	void UpdateAudio();
	void UpdateEnd();
	void RemoveGameObjects();

	static Game* ourInstance;
	unique_ptr<RenderThread> myRenderThread;
	unique_ptr<GameTaskManager> myTaskManager;

	// timer measurements
	float myFrameStart;
	float myDeltaTime;

	Camera* myCamera;
	tbb::spin_mutex myAddLock, myRemoveLock;
	unordered_map<UID, GameObject*> myGameObjects;
	queue<GameObject*> myAddQueue;
	queue<GameObject*> myRemoveQueue;
	vector<Terrain*> myTerrains;
	PhysicsWorld* myPhysWorld;

	bool myIsRunning;
	bool myShouldEnd;
	bool myIsPaused;

	// logging
	void LogToFile(string aLine);
	ofstream myFile;
};