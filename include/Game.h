#ifndef _GAME_H
#define _GAME_H

#include "Camera.h"
#include "GameObject.h"
#include <vector>

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
		size_t id;
		bool stageDone;
		size_t stage; // 0 = idle, 1 = update and cull, 2 = sort, 3 = draw
	};

	// just general settings
	const float speed = 0.2f;
	const float mouseSens = 0.2f;
};

#endif // !_GAME_H