#ifndef _GAME_H
#define _GAME_H

#include "Camera.h"
#include "GameObject.h"
#include <vector>
#include <thread>

class Game
{
public:
	Game();
	~Game();

	Game* GetInstance() { return inst; }

	void Init();
	void Update(float deltaTime);
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
		uint id, total;
		bool stageDone;
		char stage; // 0 = idle, 1 = update and cull, 2 = sort, 3 = draw
	};
	vector<thread> threads;
	void Work(ThreadInfo info);

	// just general settings
	const float speed = 0.2f;
	const float mouseSens = 0.2f;
};

#endif // !_GAME_H