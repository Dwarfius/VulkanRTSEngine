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
			threadInfos.push_back({ maxThreads, 0, Stage::Idle });
			threads[i] = thread(&Game::Work, this, i);
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
	go = new GameObject();
	go->SetModel(0);
	go->SetShader(0);
	go->SetTexture(0);
	gameObjects.push_back(go);
	go = new GameObject();
	go->SetModel(0);
	go->SetShader(0);
	go->SetTexture(0);
	gameObjects.push_back(go);

	// activating our threads
	for (uint i = 0; i < threadInfos.size(); i++)
	{
		ThreadInfo &info = threadInfos[i];
		info.stage = Stage::Update;
	}
}

void Game::Update(const float deltaTime)
{
	// notify threads of a new deltaTime
	for (uint i = 0; i < threadInfos.size(); i++)
	{
		ThreadInfo &info = threadInfos[i];
		info.deltaTime = deltaTime;
	}

	// update the camera
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
}

void Game::Render()
{
	// checking if threads are idle - means they finished submitting work
	bool threadsIdle = true;
	for (ThreadInfo info : threadInfos)
		threadsIdle &= info.stage == Stage::WaitingToSubmit;
	
	// if it's not - don't rush the display
	if (!threadsIdle)
		return;

	Graphics::Display();
	// the current render queue has been used up, we can fill it up again
	for (uint i = 0; i < threadInfos.size(); i++)
	{
		ThreadInfo &info = threadInfos[i];
		info.stage = Stage::Render;
	}
}

void Game::CleanUp()
{
	for (size_t i = 0; i < gameObjects.size(); i++)
		delete gameObjects[i];
}

void Game::Work(uint infoInd)
{
	while (running)
	{
		ThreadInfo &info = threadInfos[infoInd];
		if (info.stage == Stage::Idle)
			continue;
		
		size_t size = gameObjects.size() / info.totalThreads;
		if (size == 0)
			size = 1;
		size_t start = size * infoInd;
		size_t end = start + size;
		if (end > gameObjects.size()) // just a safety precaution
			end = gameObjects.size();

		switch (info.stage)
		{
		case Stage::Update:
			for (size_t i = start; i < end; i++)
				gameObjects[i]->Update(info.deltaTime);
			info.stage = Stage::WaitingToSubmit;
			break;
		case Stage::WaitingToSubmit:
			break;
		case Stage::Render:
			for (size_t i = start; i < end; i++)
				Graphics::Render(gameObjects[i], infoInd);
			info.stage = Stage::Update;
			break;
		}
	}
	printf("[Info] Worker thread %d ending\n", infoInd);
}