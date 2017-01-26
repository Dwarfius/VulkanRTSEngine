#ifndef _GAME_H
#define _GAME_H

#include "Camera.h"
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
	~Game();

	Game* GetInstance() { return inst; }

	void Init();
	void Update(const float deltaTime);
	void Render();
	void CleanUp();

	bool IsRunning() { return running; }
private:
	static Game *inst;

	vec2 oldMPos, curMPos;
	Camera camera;
	vector<GameObject*> gameObjects;

	bool running;
	struct ThreadInfo {
		uint totalThreads;
		float deltaTime;
		Stage stage;
	};
	vector<ThreadInfo> threadInfos;
	vector<thread> threads;
	void Work(uint infoInd);

	// just general settings
	const float speed = 0.2f;
	const float mouseSens = 0.2f;
};

#endif // !_GAME_H