#ifndef _GAME_H
#define _GAME_H

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

	bool running;
};

#endif // !_GAME_H