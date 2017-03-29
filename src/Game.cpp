#include "Game.h"
#include "GraphicsGL.h"
#include "GraphicsVK.h"
#include "Input.h"

#include "Components\Renderer.h"
#include "Components\PlayerTank.h"

#include <chrono>

// prints out the thread states to track transitions
//#define DEBUG_THREADS

// should the engine put main thread to sleep 
// for 100 nanoseconds during waiting for workers to finish
//#define USE_SLEEP

Game* Game::inst = nullptr;
Graphics *Game::graphics;

Game::Game()
{
	inst = this;

	Terrain terr;
	terr.Generate("assets/textures/heightmap.png", 0.05f, vec3(), 0.5f, 1);
	terrains.push_back(terr);

	if (isVK)
		graphics = new GraphicsVK();
	else
		graphics = new GraphicsGL();
	graphics->ResetRenderCalls();
	graphics->Init(terrains);
	
	Input::SetWindow(graphics->GetWindow());

	camera = new Camera();
	if(isVK)
		camera->InvertProj();

	grid = new Grid(vec3(-10, -10, -10), vec3(10, 10, 10), vec3(1, 1, 1));

	uint maxThreads = thread::hardware_concurrency();
	if (maxThreads > 1)
	{
		maxThreads--;
		printf("[Info] Spawning %d threads\n", maxThreads);
		threads.resize(maxThreads);
		for (uint i = 0; i < maxThreads; i++)
		{
			threadInfos.push_back({ maxThreads, Stage::Idle });
			threads[i] = thread(&Game::Work, this, i);
		}
		grid->SetTreadBufferCount(maxThreads);
	}
	else
	{
		grid->SetTreadBufferCount(1);
		printf("[Info] Using single-thread mode\n");
	}
}

void Game::Init()
{
	gameObjects.reserve(2000);
	
	GameObject *go; /* = new GameObject();
	go->AddComponent(new Renderer(0, 0, 0));
	go->GetTransform()->SetRotation(vec3(0, -90, 0));
	gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new Renderer(0, 0, 0));
	go->GetTransform()->SetPos(vec3(3, 0, 0));
	go->GetTransform()->SetRotation(vec3(0, 0, 90));
	gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new Renderer(0, 0, 0));
	go->GetTransform()->SetPos(vec3(-3, 0, 0));
	go->GetTransform()->SetRotation(vec3(90, 0, 0));
	gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new Renderer(1, 0, 2));
	go->GetTransform()->SetPos(vec3(0, 2, 0));
	gameObjects.push_back(go);*/

	go = new GameObject();
	go->AddComponent(new Renderer(1, 0, 3));
	gameObjects.push_back(go);

	go = new GameObject();
	go->AddComponent(new PlayerTank());
	go->AddComponent(new Renderer(0, 0, 2));
	go->GetTransform()->SetScale(vec3(0.5f, 0.5f, 0.5f));
	gameObjects.push_back(go);

	// activating our threads
	for (uint i = 0; i < threadInfos.size(); i++)
	{
		ThreadInfo &info = threadInfos[i];
		info.stage = Stage::Update;
	}
	graphics->BeginGather();
}

void Game::Update()
{
	frameStart = glfwGetTime();

	if (Input::GetKey(27))
	{
		running = false;
		return;
	}

	Input::Update();

	collCheckTimer += deltaTime;
	if (collCheckTimer >= collCheckRate)
	{
		const float startTime = glfwGetTime();
		//first, check if we're awaiting on the collision update
		float startWait = glfwGetTime();
		bool threadsWaitingForCol = false;
		while (!threadsWaitingForCol)
		{
#ifdef USE_SLEEP
			this_thread::sleep_for(chrono::microseconds(100));
#endif
			threadsWaitingForCol = true;
			for (ThreadInfo info : threadInfos)
				threadsWaitingForCol &= info.stage == Stage::WaitingToSubmit;
		}
		waitTime += glfwGetTime() - startWait;

		// signal workers to get start seeding objects in to grid
		for (uint i = 0; i < threadInfos.size(); i++)
		{
			ThreadInfo &info = threadInfos[i];
			info.stage = Stage::CollStage0;
		}

		startWait = glfwGetTime();
		threadsWaitingForCol = false;
		while (!threadsWaitingForCol)
		{
#ifdef USE_SLEEP
			this_thread::sleep_for(chrono::microseconds(100));
#endif
			threadsWaitingForCol = true;
			for (ThreadInfo info : threadInfos)
				threadsWaitingForCol &= info.stage == Stage::WaitingToSubmit;
		}
		waitTime += glfwGetTime() - startWait;

		// flush the grid in respective locations on the main thread
		grid->Flush();

		// signal workers to get start processing grid cells
		for (uint i = 0; i < threadInfos.size(); i++)
		{
			ThreadInfo &info = threadInfos[i];
			info.stage = Stage::CollStage1;
		}

		const float endTime = glfwGetTime();
		collCheckTime = endTime - startTime;

		// reset the timer of collision checking
		collCheckTimer = 0;
	}
}

void Game::Render()
{
	// checking if threads are waiting to submit - means they finished submitting work
	float startWait = glfwGetTime();
	bool threadsIdle = false;
	while (!threadsIdle)
	{
#ifdef USE_SLEEP
		this_thread::sleep_for(chrono::microseconds(100));
#endif
		threadsIdle = true;
		for (ThreadInfo info : threadInfos)
			threadsIdle &= info.stage == Stage::WaitingToSubmit;
	}
	waitTime += glfwGetTime() - startWait;

	const float startTime = glfwGetTime();
	// safe place to change up things
	if (Input::GetKeyPressed(GLFW_KEY_F1))
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

	float renderStart = glfwGetTime();
	graphics->Display();
	float renderLength = glfwGetTime() - renderStart;

	printf("[Info] Render calls: %d\n", graphics->GetRenderCalls());
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
	deltaTime = glfwGetTime() - frameStart;
	printf("[Info] Render Rendering: %.2fms, Collisions: %.2fms, Wait: %.2fms (Total: %.2fms)\n", renderLength * 1000.f, collCheckTime * 1000.f, waitTime * 1000.f, deltaTime * 1000.f);

	// resetting our timers
	collCheckTime = 0;
	waitTime = 0;
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
	delete grid;
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
#ifdef DEBUG_THREADS
			printf("[Info] Update(%d)\n", infoInd);
#endif
			for (size_t i = start; i < end; i++)
			{
				gameObjects[i]->SetIndex(i);
				gameObjects[i]->Update(deltaTime);
			}
			info.stage = Stage::WaitingToSubmit;
			break;
		case Stage::CollStage0:
#ifdef DEBUG_THREADS
			printf("[Info] CollStage0(%d)\n", infoInd);
#endif
			for (size_t i = start; i < end; i++)
				grid->Add(gameObjects[i], infoInd);
			info.stage = Stage::WaitingToSubmit;
			break;
		case Stage::CollStage1:
#ifdef DEBUG_THREADS
			printf("[Info] CollStage1(%d)\n", infoInd);
#endif
			// process the actual collisions here

			info.stage = Stage::WaitingToSubmit;
			break;
			break;
		case Stage::WaitingToSubmit:
#ifdef DEBUG_THREADS
			printf("[Info] WaitingToSubmit(%d)\n", infoInd);
#endif
			// we have to wait until the render thread finishes processing the submitted commands
			// otherwise we'll overflow the command buffers
			// in case collision check was triggered, jump back - handled in Update
			break;
		case Stage::Render:
#ifdef DEBUG_THREADS
			printf("[Info] Render(%d)\n", infoInd);
#endif
			for (size_t i = start; i < end; i++)
				graphics->Render(camera, gameObjects[i], infoInd);
			info.stage = Stage::Update;
			break;
		}
	}
	ThreadInfo &info = threadInfos[infoInd];
	info.stage = Stage::WaitingToSubmit;
	printf("[Info] Worker thread %d ending\n", infoInd);
}