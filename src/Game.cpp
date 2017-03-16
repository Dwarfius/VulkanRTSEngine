#include "Game.h"
#include "GraphicsGL.h"
#include "GraphicsVK.h"
#include "Input.h"

#include "Components\Renderer.h"
#include "Components\PlayerTank.h"

Game* Game::inst = nullptr;
Graphics *Game::graphics;

Game::Game()
{
	inst = this;

	Terrain terr;
	terr.Generate("assets/textures/heightmap.png", 0.2f, vec3(), 2);
	terrains.push_back(terr);

	if (isVK)
		graphics = new GraphicsVK();
	else
		graphics = new GraphicsGL();
	graphics->Init(terrains);
	
	Input::SetWindow(graphics->GetWindow());

	camera = new Camera();
	if(isVK)
		camera->InvertProj();

	uint maxThreads = thread::hardware_concurrency();
	if (maxThreads > 1)
	{
		maxThreads--;
		printf("[Info] Spawning %d threads\n", maxThreads);
		threads.resize(maxThreads);
		for (uint i = 0; i < maxThreads; i++)
		{
			threadInfos.push_back({ maxThreads, Stage::Idle, 0 });
			threads[i] = thread(&Game::Work, this, i);
		}
	}
	else
		printf("[Info] Using single-thread mode\n");
}

void Game::Init()
{
	gameObjects.reserve(2000);
	
	GameObject *go = new GameObject();
	go->AddComponent(new Renderer(0, 0, 0));
	go->GetTransform()->SetRotation(vec3(0, -90, 0));
	//gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new Renderer(0, 0, 0));
	go->GetTransform()->SetPos(vec3(3, 0, 0));
	go->GetTransform()->SetRotation(vec3(0, 0, 90));
	//gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new Renderer(0, 0, 0));
	go->GetTransform()->SetPos(vec3(-3, 0, 0));
	go->GetTransform()->SetRotation(vec3(90, 0, 0));
	//gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new Renderer(1, 0, 2));
	go->GetTransform()->SetPos(vec3(0, 2, 0));
	//gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new Renderer(2, 0, 1));
	gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new PlayerTank());
	go->AddComponent(new Renderer(1, 0, 2));
	go->GetTransform()->SetScale(vec3(0.5f, 0.5f, 0.5f));
	gameObjects.push_back(go);

	// activating our threads
	for (uint i = 0; i < threadInfos.size(); i++)
	{
		ThreadInfo &info = threadInfos[i];
		info.stage = Stage::Update;
	}
	// setting the up-to-date time
	oldTime = glfwGetTime();

	graphics->BeginGather();
}

void Game::Update()
{
	if (Input::GetKey(27))
	{
		running = false;
		return;
	}

	// checking if threads are idle - means we can start a new frame
	bool threadsIdle = true;
	for (ThreadInfo info : threadInfos)
		threadsIdle &= info.stage == Stage::Idle;

	// if it's not - don't rush it
	if (!threadsIdle)
		return;

	const float time = glfwGetTime();
	const float deltaTime = time - oldTime;
	oldTime = time;
	Input::Update();

	for (uint i = 0; i < threadInfos.size(); i++)
	{
		ThreadInfo &info = threadInfos[i];
		info.deltaTime = deltaTime;
		info.stage = Stage::Update;
	}
}

void Game::Render()
{
	// checking if threads are waiting to submit - means they finished submitting work
	bool threadsIdle = true;
	for (ThreadInfo info : threadInfos)
		threadsIdle &= info.stage == Stage::WaitingToSubmit;
	
	// if it's not - don't rush the display
	if (!threadsIdle)
		return;

	// safe place to change up things
	if (glfwGetKey(graphics->GetWindow(), GLFW_KEY_F1) == GLFW_PRESS)
	{
		printf("[Info] Switching renderer...\n");
		isVK = !isVK;

		graphics->CleanUp();
		delete graphics;

		if (isVK)
			graphics = new GraphicsVK();
		else
			graphics = new GraphicsGL();
		graphics->Init(terrains);
		camera->InvertProj();
	}

	graphics->Display();

	//printf("[Info] Render calls: %d\n", graphics->GetRenderCalls());
	graphics->ResetRenderCalls();

	// update the mvp
	camera->Recalculate();
	
	// the current render queue has been used up, we can fill it up again
	graphics->BeginGather();
	for (uint i = 0; i < threadInfos.size(); i++)
	{
		ThreadInfo &info = threadInfos[i];
		info.stage = Stage::Render;
	}
}

void Game::CleanUp()
{
	// checking if threads are idle - means they finished submitting work and we can clean em up
	bool threadsIdle = false;
	while (!threadsIdle)
	{
		threadsIdle = true;
		for (ThreadInfo info : threadInfos)
			threadsIdle &= info.stage == Stage::WaitingToSubmit;
	}

	// we can mark that the engine is done - wrap the threads
	running = false;
	for (size_t i = 0; i < threads.size(); i++)
		threads[i].join();

	for (size_t i = 0; i < gameObjects.size(); i++)
		delete gameObjects[i];

	graphics->CleanUp();

	delete camera;
}

Terrain* Game::GetTerrain(vec3 pos)
{
	return &terrains[0];
}

void Game::Work(uint infoInd)
{
	while (running)
	{
		ThreadInfo &info = threadInfos[infoInd];
		if (info.stage == Stage::Idle)
			continue;

		size_t size = ceil(gameObjects.size() / (float)info.totalThreads);
		size_t start = size * infoInd;
		size_t end = start + size;
		if (end > gameObjects.size()) // just a safety precaution
			end = gameObjects.size();

		switch (info.stage)
		{
		case Stage::Update:
			for (size_t i = start; i < end; i++)
			{
				gameObjects[i]->SetIndex(i);
				gameObjects[i]->Update(info.deltaTime);
			}
			info.stage = Stage::WaitingToSubmit;
			break;
		case Stage::WaitingToSubmit:
			// we have to wait until the render thread finishes processing the submitted commands
			// otherwise we'll overflow the command buffers
			break;
		case Stage::Render:
			for (size_t i = start; i < end; i++)
				graphics->Render(camera, gameObjects[i], infoInd);
			info.stage = Stage::Idle;
			break;
		}
	}
	ThreadInfo &info = threadInfos[infoInd];
	info.stage = Stage::WaitingToSubmit;
	printf("[Info] Worker thread %d ending\n", infoInd);
}