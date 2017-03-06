#ifndef _GAME_H
#define _GAME_H

#include "Camera.h"
#include "Graphics.h"
#include "GameObject.h"
#include <vector>
#include <thread>

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

	static Graphics* GetGraphics() { return graphics; }
private:
	static Game *inst;
	static Graphics *graphics;
	bool isVK = false;

	float oldTime;
	vec2 oldMPos;
	Camera *camera;
	vector<GameObject*> gameObjects;

	bool running;
	struct ThreadInfo {
		uint totalThreads;
		Stage stage;
		float deltaTime;
	};
	vector<ThreadInfo> threadInfos;
	vector<thread> threads;
	void Work(uint infoInd);

	// just general settings
	const float speed = 1;
	const float mouseSens = 0.2f;
};

#endif // !_GAME_H