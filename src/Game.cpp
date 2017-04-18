#include "Game.h"
#include "GraphicsGL.h"
#include "GraphicsVK.h"
#include "Input.h"
#include "Audio.h"

#include "Components\Renderer.h"
#include "Components\PlayerTank.h"
#include "Components\Tank.h"
#include "Components\GameMode.h"

#include <chrono>

// prints out the thread states to track transitions
//#define DEBUG_THREADS

// should the engine put main thread to sleep instead of yielding it
// for 1 nanosecond during waiting for workers to finish
// yield leads to a smoother draw-rate, also more optimal for Vulkan judging by test
//#define USE_SLEEP

// a define controlling the print out of some general information
//#define PRINT_GAME_INFO

Game* Game::inst = nullptr;
Graphics *Game::graphics;

Game::Game()
{
	inst = this;

	file.open("log.csv");
	if (!file.is_open())
		printf("[Warning] Log file didn't open\n");
	LogToFile("Time, Render(ms), Physics(ms), Idle(ms), Total(ms), Visible, Total");

	Audio::Init();
	//Audio::SetMusicTrack(2);

	Terrain terr;
	terr.Generate("assets/textures/heightmap.png", 0.3f, vec3(), 2.f, 1);
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

	grid = new Grid(vec3(-50, 0, -50), vec3(50, 1, 50), vec3(1, 1, 1));

	uint maxThreads = thread::hardware_concurrency();
	if (maxThreads > 1)
	{
		maxThreads--;
		printf("[Info] Spawning %d threads\n", maxThreads);
		threads.resize(maxThreads);
		for (uint i = 0; i < maxThreads; i++)
		{
			threadInfos.push_back({ maxThreads, Stage::Idle, 0, 0 });
			threads[i] = thread(&Game::Work, this, i);
		}
		grid->SetTreadBufferCount(maxThreads);
	}
	else
	{
		printf("[Error] Machine must have more than 1 hardware thread to run\n");
		running = false;
		// grid->SetTreadBufferCount(1);
		// printf("[Info] Using single-thread mode\n");
	}

	aliveGOs.reserve(maxObjects);
}

void Game::Init()
{
	gameObjects.reserve(maxObjects);
	aliveGOs.reserve(maxObjects);
	
	GameObject *go; 

	// terrain
	go = Instantiate();
	go->AddComponent(new Renderer(1, 0, 3));
	go->SetCollisionsEnabled(false);

	// player
	go = Instantiate();
	go->AddComponent(new GameMode());

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
		paused = running = false;
		return;
	}

	if (Input::GetKeyPressed('B'))
		paused = !paused;
	if (paused)
		return;

	// audio controls
	if (Input::GetKeyPressed('U'))
		Audio::IncreaseVolume();
	if (Input::GetKeyPressed('J'))
		Audio::DecreaseVolume();

	if (Input::GetKeyPressed('I'))
		sensitivity += 0.3f;
	if (Input::GetKeyPressed('K') && sensitivity >= 0.3f)
		sensitivity -= 0.3f;

	collCheckTimer += deltaTime;
	if (collCheckTimer >= collCheckRate)
	{
		const float startTime = glfwGetTime();
		//first, check if we're awaiting on the collision update
		float startWait = glfwGetTime();
		bool threadsWaitingForCol = false;
		while (!threadsWaitingForCol)
		{
			threadsWaitingForCol = true;
			for (ThreadInfo info : threadInfos)
				threadsWaitingForCol &= info.stage == Stage::WaitingToSubmit;

			if (!threadsWaitingForCol)
			{
#ifdef USE_SLEEP
				this_thread::sleep_for(chrono::microseconds(1));
#else
				this_thread::yield();
#endif
			}
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
			threadsWaitingForCol = true;
			for (ThreadInfo info : threadInfos)
				threadsWaitingForCol &= info.stage == Stage::WaitingToSubmit;

			if (!threadsWaitingForCol)
			{
#ifdef USE_SLEEP
				this_thread::sleep_for(chrono::microseconds(1));
#else
				this_thread::yield();
#endif
			}
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
		threadsIdle = true;
		for (ThreadInfo info : threadInfos)
			threadsIdle &= info.stage == Stage::WaitingToSubmit;

		if (!threadsIdle)
		{
#ifdef USE_SLEEP
			this_thread::sleep_for(chrono::microseconds(1));
#else
			this_thread::yield();
#endif
		}
	}
	
	waitTime += glfwGetTime() - startWait;

	const float startTime = glfwGetTime();
	// safe place to change up things
	if (Input::GetKeyPressed('G'))
	{
		printf("[Info] Switching renderer...\n");
		isVK = !isVK;

		graphics->CleanUp();
		delete graphics;

		printf("\n");
		if (isVK)
			graphics = new GraphicsVK();
		else
			graphics = new GraphicsGL();
		graphics->Init(terrains);
		camera->InvertProj();
		Input::SetWindow(graphics->GetWindow());
	}

	// flush the input buffers
	Input::Update();
	if (paused)
		return;

	float renderStart = glfwGetTime();
	graphics->Display();
	float renderLength = glfwGetTime() - renderStart;

#ifdef PRINT_GAME_INFO
	printf("[Info] Render calls: %d (of %zd total)\n", graphics->GetRenderCalls(), gameObjects.size());
#endif
	graphics->ResetRenderCalls();

	// update the mvp
	camera->Recalculate();
	
	// this is the place to clear out our gameobjects
	// the render calls that have been scheduled have been consumed
	// so no extra references are left
	// can't use parallel_for because it seems to break down the rendering loop
	// because there's no way that I know of atm to wait until the parallel_for finishes
	for(GameObject *go : gameObjects) 
	{
		if (!go->IsDead())
			aliveGOs.push_back(go);
		else
			delete go;
	}

	// swap
	size_t objsDeleted = gameObjects.size() - aliveGOs.size();
	gameObjects.swap(aliveGOs);
	//printf("[Info] was %zd, after delete %zd\n", aliveGOs.size(), gameObjects.size());
	aliveGOs.clear(); // clean it up for the next iteration to fill it out

	// play out the audio
	Audio::PlayQueue(camera->GetTransform());

	// the current render queue has been used up, we can fill it up again
	graphics->BeginGather();
	for (uint i = 0; i < threadInfos.size(); i++)
	{
		ThreadInfo &info = threadInfos[i];
		info.stage = Stage::Render;
	}
	deltaTime = glfwGetTime() - frameStart;
#ifdef PRINT_GAME_INFO
	printf("[Info] Render Rendering: %.2fms, Collisions: %.2fms, Wait: %.2fms (Total: %.2fms)\n", renderLength * 1000.f, collCheckTime * 1000.f, waitTime * 1000.f, deltaTime * 1000.f);
#endif

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

	LogToFile("Ending\n");
	if(file.is_open())
		file.close();

	// we can mark that the engine is done - wrap the threads
	running = false;
	for (size_t i = 0; i < threads.size(); i++)
		threads[i].join();
	
	for (GameObject *go : gameObjects)
		delete go;
	gameObjects.clear();

	graphics->CleanUp();

	delete camera;
	delete grid;
}

GameObject* Game::Instantiate(vec3 pos, vec3 rot, vec3 scale)
{
	if (gameObjects.size() == maxObjects - 1)
		return nullptr;

	GameObject *go = new GameObject(pos, rot, scale);
	gameObjects.push_back(go);

	return go;
}

Terrain* Game::GetTerrain(vec3 pos)
{
	return &terrains[0];
}

float Game::GetTime()
{
	return glfwGetTime();
}

void Game::Work(uint infoInd)
{
	while (running)
	{
		ThreadInfo &info = threadInfos[infoInd];
		if (info.stage == Stage::Idle)
		{
#ifdef USE_SLEEP
			this_thread::sleep_for(chrono::microseconds(1));
#else
			this_thread::yield();
#endif
			continue;
		}

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
				GameObject *go = gameObjects[i];
				go->SetIndex(i);
				go->Update(deltaTime);
			}
			info.start = start;
			info.end = end;
			info.stage = Stage::WaitingToSubmit;
			break;
		case Stage::CollStage0: // collision preprocessing - sorting of objects to grid
#ifdef DEBUG_THREADS
			printf("[Info] CollStage0(%d)\n", infoInd);
#endif
			for (size_t i = start; i < end; i++)
				grid->Add(gameObjects[i], infoInd);
			info.stage = Stage::WaitingToSubmit;
			break;
		case Stage::CollStage1: // actual collision processing
		{
#ifdef DEBUG_THREADS
			printf("[Info] CollStage1(%d)\n", infoInd);
#endif
			// figure out which cells to process
			size_t size = ceil(grid->GetTotal() / (float)info.totalThreads);
			size_t start = size * infoInd;
			size_t end = start + size;
			if (end > grid->GetTotal()) // just a safety precaution
				end = grid->GetTotal();
			for (size_t i = start; i < end; i++)
			{
				vector<GameObject*> *cell = grid->GetCell(i);
				size_t cellSize = cell->size();
				for (size_t j = 0; j < cellSize; j++)
				{
					// first, check terrain collision
					GameObject *go = cell->at(j);
					Transform *t = go->GetTransform();
					Terrain *terrain = GetTerrain(t->GetPos());
					if (terrain->Collides(t->GetPos(), go->GetRadius()))
						go->CollidedWithTerrain();

					for (size_t k = j + 1; k < cellSize; k++)
					{
						GameObject *other = cell->at(k);
						if (GameObject::Collide(go, other))
						{
							go->CollidedWithGO(other);
							other->CollidedWithGO(go);
						}
					}
				}
			}

			info.stage = Stage::WaitingToSubmit;
		}
		break;
		case Stage::WaitingToSubmit:
#ifdef DEBUG_THREADS
			printf("[Info] WaitingToSubmit(%d)\n", infoInd);
#endif
#ifdef USE_SLEEP
			this_thread::sleep_for(chrono::microseconds(1));
#else
			this_thread::yield();
#endif
			// we have to wait until the render thread finishes processing the submitted commands
			// otherwise we'll overflow the command buffers
			// in case collision check was triggered, jump back - handled in Update
			break;
		case Stage::Render:
			{
			size_t start = info.start;
			size_t end = info.end < gameObjects.size() ? info.end : gameObjects.size();
#ifdef DEBUG_THREADS
				printf("[Info] Render(%d) from %zd to %zd\n", infoInd, start, end);
#endif
				//printf("[Info] Render(%d) from %zd to %zd\n", infoInd, start, end);
				for (size_t i = start; i < end; i++)
				{
					GameObject *go = gameObjects[i];
					if (go->GetIndex() != numeric_limits<size_t>::max() &&
						camera->CheckSphere(go->GetTransform()->GetPos(), go->GetRadius()))
						graphics->Render(camera, go, infoInd);
				}
				info.stage = Stage::Update;
			}
			break;
		}
	}
	ThreadInfo &info = threadInfos[infoInd];
	info.stage = Stage::WaitingToSubmit;
	printf("[Info] Worker thread %d ending\n", infoInd);
}

void Game::LogToFile(string s)
{
	if (file.is_open())
		file << s << endl;
}