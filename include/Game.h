#ifndef _GAME_H
#define _GAME_H

#include "Camera.h"
#include "Graphics.h"
#include "GameObject.h"
#include <vector>
#include <thread>
#include "Terrain.h"

// have to define it outside of Game
// it messes up with method declaration
enum Stage {
	Idle,
	Update,
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
	size_t GetGameObjectCount() { return gameObjects.size(); }
	const static uint32_t maxObjects = 4000;

	Camera* GetCamera() { return camera; }
	Terrain *GetTerrain(vec3 pos);

	static Graphics* GetGraphics() { return graphics; }
private:
	static Game *inst;
	static Graphics *graphics;
	bool isVK = false;

	float oldTime;
	Camera *camera;
	vector<GameObject*> gameObjects;
	vector<Terrain> terrains;

	bool running;
	struct ThreadInfo {
		uint totalThreads;
		Stage stage;
		float deltaTime;
	};
	vector<ThreadInfo> threadInfos;
	vector<thread> threads;
	void Work(uint infoInd);
};

#endif // !_GAME_H