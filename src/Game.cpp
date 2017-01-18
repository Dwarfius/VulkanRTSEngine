#include "Game.h"
#include "Graphics.h"

Game* Game::inst = nullptr;

Game::Game()
{
	inst = this;

	uint maxThreads = thread::hardware_concurrency();
	if (maxThreads > 1)
	{
		maxThreads--;
		printf("[Info] Spawning %d threads\n", maxThreads);
		threads.resize(maxThreads);
		for (uint i = 0; i < maxThreads; i++)
		{
			ThreadInfo info{ i, maxThreads, false, 0 };
			threads[i] = thread(&Game::Work, this, info);
		}
	}
	else
		printf("[Info] Using single-thread mode\n");
}

Game::~Game()
{
	running = false;
	for (size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

void Game::Init()
{
	gameObjects.reserve(2000);
	GameObject *go = new GameObject();
	go->SetModel(0);
	go->SetShader(0);
	go->SetTexture(0);
	gameObjects.push_back(go);
}

void Game::Update(float deltaTime)
{
	// first we update the camera
	vec3 forward = camera.GetForward();
	vec3 right = camera.GetRight();
	vec3 up = camera.GetUp();
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_W) == GLFW_REPEAT)
		camera.Translate(forward * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_S) == GLFW_REPEAT)
		camera.Translate(-forward * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_D) == GLFW_REPEAT)
		camera.Translate(right * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_A) == GLFW_REPEAT)
		camera.Translate(-right * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_Q) == GLFW_REPEAT)
		camera.Translate(-up * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_E) == GLFW_REPEAT)
		camera.Translate(up * deltaTime * speed);

	oldMPos = curMPos;
	double x, y;
	glfwGetCursorPos(Graphics::GetWindow(), &x, &y);
	curMPos = vec2(x, y);
	vec2 deltaPos = curMPos - oldMPos;
	camera.Rotate((float)deltaPos.x * mouseSens, (float)-deltaPos.y * mouseSens);

	// [NYI]start updating the objects
	// ...
}

void Game::Render()
{
	// multithread this
	for (auto go : gameObjects)
		Graphics::Render(go);

	Graphics::Display();
}

void Game::CleanUp()
{
	for (size_t i = 0; i < gameObjects.size(); i++)
		delete gameObjects[i];
}

void Game::Work(ThreadInfo info)
{
	while (running)
	{
		if (info.stageDone || info.stage == 0)
			continue;

		switch (info.stage)
		{
		case 1:
			break;
		default:
			break;
		}
	}
	printf("[Info] Worker thread %d ending\n", info.id);
}