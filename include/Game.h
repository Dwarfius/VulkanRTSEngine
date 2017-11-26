#pragma once

#include "Terrain.h"
#include "GameTaskManager.h"
#include "RenderThread.h"

class Camera;
class GameObject;
class Grid;
class Graphics;

class Game
{
public:
	Game();
	~Game() { }

	static Game* GetInstance() { return inst; }

	void Init();
	void RunTaskGraph();
	void CleanUp();

	bool IsRunning() { return running; }
	void EndGame() { shouldEnd = true; }
	bool IsPaused() { return paused; }

	const tbb::concurrent_vector<GameObject*>& GetGameObjects() const { return gameObjects; }
	size_t GetGameObjectCount() { return gameObjects.size(); }
	GameObject* Instantiate(vec3 pos = vec3(), vec3 rot = vec3(), vec3 scale = vec3(1));
	const static uint32_t maxObjects = 4000;

	Camera* GetCamera() const { return camera; }
	const Terrain* GetTerrain(vec3 pos) const;

	// utility method for accessing the time across game
	float GetTime() const;
	float GetSensitivity() const { return sensitivity; }

	// utility for tracking IDs
	void ReturnId(size_t id);
	size_t ClaimId();

	Graphics* GetGraphicsRaw();
	const Graphics* GetGraphics() const;

private:
	void UpdateInput();
	void Update();
	void CollisionUpdate();
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

	bool isVK = true;

	// timer measurements
	float frameStart = 0;
	float deltaTime = 0;

	Camera *camera;
	// TODO: replace with a std::unordered_map
	tbb::concurrent_vector<GameObject*> gameObjects;
	// don't use this vector directly, it's always empty
	// it's here to avoid constant memory allocations
	// to store maxObjects objects
	tbb::concurrent_vector<GameObject*> aliveGOs;
	vector<Terrain> terrains;
	Grid *grid;

	bool running = true, shouldEnd = false;
	bool paused = false;
	tbb::concurrent_queue<size_t> ids;
	vector<thread> threads;

	// logging
	void LogToFile(string s);
	ofstream file;
};