#ifndef _GAME_H
#define _GAME_H

#include "Camera.h"
#include "Graphics.h"
#include "GameObject.h"
#include <vector>
#include <thread>
#include "Terrain.h"
#include "Grid.h"
#include <fstream>

#include <tbb\tbb.h>
#undef max

// have to define it outside of Game
// it messes up with method declaration
enum Stage {
	Idle,
	Update,
	CollStage0, // pre-seed
	CollStage1, // cell-process
	WaitingToSubmit,
	Render
};

class Game
{
public:
	Game();
	~Game() { }

	static Game* GetInstance() { return inst; }

	void Init();
	void Update();
	void Render();
	void CleanUp();

	bool IsRunning() { return running; }
	bool IsPaused() { return paused; }
	size_t GetGameObjectCount() { return gameObjects.size(); }
	GameObject* Instantiate(vec3 pos = vec3(), vec3 rot = vec3(), vec3 scale = vec3(1));
	const static uint32_t maxObjects = 4000;

	Camera* GetCamera() { return camera; }
	Terrain *GetTerrain(vec3 pos);

	// utility method for accessing the time across game
	float GetTime();
	float GetSensitivity() { return sensitivity; }

	void ReturnId(size_t id);
	size_t ClaimId();

	static Graphics* GetGraphics() { return graphics; }

private:
	const float collCheckRate = 0.033f; //30col/s
	float collCheckTimer = 0;

	float sensitivity = 2.5f;

	static Game *inst;
	static Graphics *graphics;

	bool isVK = true;

	// timer measurements
	float frameStart = 0;
	float deltaTime = 0;
	float collCheckTime = 0;
	float waitTime = 0;

	Camera *camera;
	tbb::concurrent_vector<GameObject*> gameObjects;
	// don't use this vector directly, it's always empty
	// it's here to avoid constant memory allocations
	// to store maxObjects objects
	tbb::concurrent_vector<GameObject*> aliveGOs;
	vector<Terrain> terrains;
	Grid *grid;

	bool running = true;
	bool paused = false;
	struct ThreadInfo {
		uint totalThreads;
		Stage stage;
		size_t start, end;
	};
	
	tbb::concurrent_queue<size_t> ids;
	vector<ThreadInfo> threadInfos;
	vector<thread> threads;
	void Work(uint infoInd);

	// logging
	void LogToFile(string s);
	ofstream file;
};

#endif // !_GAME_H