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

	Game(ReportError reporterFunc);
	~Game();

	static Game* GetInstance() { return inst; }

	void Init();
	void RunMainThread();
	void RunTaskGraph();
	void CleanUp();

	bool IsRunning() const;
	void EndGame() { shouldEnd = true; }
	bool IsPaused() const { return paused; }
	GLFWwindow* GetWindow() const;

	const unordered_map<UID, GameObject*>& GetGameObjects() const { return gameObjects; }
	size_t GetGameObjectCount() const { return gameObjects.size(); }
	GameObject* Instantiate(glm::vec3 pos = glm::vec3(), glm::vec3 rot = glm::vec3(), glm::vec3 scale = glm::vec3(1));
	const static uint32_t maxObjects = 4000;

	Camera* GetCamera() const { return camera; }
	const Terrain* GetTerrain(glm::vec3 pos) const;

	// utility method for accessing the time across game
	float GetTime() const;
	float GetSensitivity() const { return sensitivity; }

	Graphics* GetGraphics() { return renderThread->GetGraphicsRaw(); }
	const Graphics* GetGraphics() const { return renderThread->GetGraphics(); }

	void RemoveGameObject(GameObject* go);

	static bool goDeleteEnabled;

private:
	void AddGameObjects();
	void UpdateInput();
	void Update();
	void PhysicsUpdate();
	void Render();
	void UpdateAudio();
	void UpdateEnd();
	void RemoveGameObjects();

	const float collCheckRate = 0.033f; //30col/s
	float collCheckTimer = 0;

	float sensitivity = 2.5f;

	static Game* inst;
	unique_ptr<RenderThread> renderThread;
	unique_ptr<GameTaskManager> taskManager;

	const bool isVK = true;

	// timer measurements
	float frameStart = 0;
	float deltaTime = 0;

	Camera *camera;
	tbb::spin_mutex addLock, removeLock;
	unordered_map<UID, GameObject*> gameObjects;
	queue<GameObject*> addQueue;
	queue<GameObject*> removeQueue;
	vector<Terrain*> terrains;
	PhysicsWorld* myPhysWorld;

	bool running = true, shouldEnd = false;
	bool paused = false;

	// logging
	void LogToFile(string s);
	ofstream file;
};