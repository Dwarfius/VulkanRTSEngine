#include "Common.h"
#include "Game.h"
#include "Grid.h"
#include "Graphics.h"
#include "Input.h"
#include "Audio.h"
#include "Camera.h"
#include "Terrain.h"
#include "GameObject.h"

#include "Components\Renderer.h"
#include "Components\PlayerTank.h"
#include "Components\Tank.h"
#include "Components\GameMode.h"

// prints out the thread states to track transitions
//#define DEBUG_THREADS

// should the engine put main thread to sleep instead of yielding it
// for 1 nanosecond during waiting for workers to finish
// yield leads to a smoother draw-rate, also more optimal for Vulkan judging by test
//#define USE_SLEEP

Game* Game::inst = nullptr;
bool Game::goDeleteEnabled = false;

Game::Game()
{
	inst = this;
	UID::Init();
	
	file.open(isVK ? "logVK.csv" : "logGL.csv");
	if (!file.is_open())
		std::printf("[Warning] Log file didn't open\n");

	//Audio::Init();
	//Audio::SetMusicTrack(2);

	Terrain terr;
	terr.Generate("assets/textures/heightmap.png", 0.3f, vec3(), 2.f, 1);
	terrains.push_back(terr);

	renderThread = make_unique<RenderThread>();

	camera = new Camera(Graphics::GetWidth(), Graphics::GetHeight());

	grid = new Grid(vec3(-50, 0, -50), vec3(50, 1, 50), vec3(1, 1, 1));

	taskManager = make_unique<GameTaskManager>();
}

void Game::Init()
{
	gameObjects.reserve(maxObjects);

	renderThread->Init(isVK, &terrains);

	GameObject *go; 

	// terrain
	go = Instantiate();
	go->AddComponent(new Renderer(1, 0, 3));
	go->SetCollisionsEnabled(false);

	// player
	go = Instantiate();
	go->AddComponent(new GameMode());

	GameTask task(GameTask::UpdateInput, bind(&Game::UpdateInput, this));
	taskManager->AddTask(task);

	task = GameTask(GameTask::AddGameObjects, bind(&Game::AddGameObjects, this));
	taskManager->AddTask(task);

	task = GameTask(GameTask::GameUpdate, bind(&Game::Update, this));
	task.AddDependency(GameTask::UpdateInput);
	task.AddDependency(GameTask::AddGameObjects);
	taskManager->AddTask(task);

	// TODO: need to fix collisions, start using Bullet
	task = GameTask(GameTask::CollisionUpdate, bind(&Game::CollisionUpdate, this));
	task.AddDependency(GameTask::GameUpdate);
	taskManager->AddTask(task);

	task = GameTask(GameTask::UpdateEnd, bind(&Game::UpdateEnd, this));
	task.AddDependency(GameTask::GameUpdate);
	task.AddDependency(GameTask::CollisionUpdate);
	taskManager->AddTask(task);

	task = GameTask(GameTask::RemoveGameObjects, bind(&Game::RemoveGameObjects, this));
	task.AddDependency(GameTask::GameUpdate);
	taskManager->AddTask(task);

	task = GameTask(GameTask::Render, bind(&Game::Render, this));
	task.AddDependency(GameTask::RemoveGameObjects);
	taskManager->AddTask(task);

	// TODO: will need to fix up audio
	task = GameTask(GameTask::UpdateAudio, bind(&Game::UpdateAudio, this));
	task.AddDependency(GameTask::UpdateEnd);
	taskManager->AddTask(task);

	taskManager->ResolveDependencies();
	taskManager->Run();
}

void Game::RunMainThread()
{
	if (renderThread->HasWork())
	{
		renderThread->InternalLoop();

		// TODO: need a semaphore for this, to disconnect from the render thread
		RunTaskGraph();
	}
}

void Game::RunTaskGraph()
{
	taskManager->Run();
}

void Game::CleanUp()
{
	if (file.is_open())
		file.close();

	// we can mark that the engine is done - wrap the threads
	running = false;
	goDeleteEnabled = true;
	for (auto pair : gameObjects)
		delete pair.second;
	gameObjects.clear();

	delete camera;
	delete grid;
}

void Game::AddGameObjects()
{
	tbb::spin_mutex::scoped_lock spinlock(addLock);
	while (addQueue.size())
	{
		GameObject* go = addQueue.front();
		addQueue.pop();
	}
}

void Game::UpdateInput()
{
	Input::Update();
}

void Game::Update()
{
	{
		float newTime = static_cast<float>(glfwGetTime());
		deltaTime = newTime - frameStart;
		frameStart = newTime;
	}

	if (Input::GetKey(27) || shouldEnd)
	{
		paused = running = false;
		return;
	}

	if (Input::GetKeyPressed('B'))
	{
		paused = !paused;
		printf("pause: %d\n", paused);
	}
	if (paused)
		return;

	if (Input::GetKeyPressed('I'))
		sensitivity += 0.3f;
	if (Input::GetKeyPressed('K') && sensitivity >= 0.3f)
		sensitivity -= 0.3f;

	// TODO: at the moment all gameobjects don't have cross-synchronization, so will need to fix this up
	for (auto pair : gameObjects)
	{
		pair.second->Update(deltaTime);
	}
}

void Game::CollisionUpdate()
{
	// TODO: need to fix up collision detection
	return;

	collCheckTimer += deltaTime;
	if (collCheckTimer >= collCheckRate)
	{
		const float startTime = static_cast<float>(glfwGetTime());
		
		for (auto pair : gameObjects)
			grid->Add(pair.second, 0);

		// flush the grid in respective locations on the main thread
		grid->Flush();

		// TODO: parrallelize it
		// figure out which cells to process
		for (size_t i = 0, end = grid->GetTotal(); i < end; i++)
		{
			vector<GameObject*> *cell = grid->GetCell(i);
			size_t cellSize = cell->size();
			for (size_t j = 0; j < cellSize; j++)
			{
				// first, check terrain collision
				GameObject *go = cell->at(j);
				Transform *t = go->GetTransform();
				const Terrain *terrain = GetTerrain(t->GetPos());
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

		// reset the timer of collision checking
		collCheckTimer = 0;
	}
}

void Game::Render()
{
	for (const pair<UID, GameObject*>& elem : gameObjects)
	{
		renderThread->AddRenderable(elem.second);
	}

	// we have to wait until the render thread finishes processing the submitted commands
	// otherwise we'll screw the command buffers
	while (renderThread->IsBusy())
		tbb::this_tbb_thread::yield();

	renderThread->Work();
}

void Game::UpdateAudio()
{
	// audio controls
	//if (Input::GetKeyPressed('U'))
	//Audio::IncreaseVolume();
	//if (Input::GetKeyPressed('J'))
	//Audio::DecreaseVolume();

	// play out the audio
	//Audio::PlayQueue(camera->GetTransform());
}

void Game::UpdateEnd()
{
	Input::PostUpdate();
}

void Game::RemoveGameObjects()
{
	goDeleteEnabled = true;
	tbb::spin_mutex::scoped_lock spinlock(removeLock);
	while (removeQueue.size())
	{
		GameObject* go = removeQueue.front();
		gameObjects.erase(go->GetUID());
		delete go;
		removeQueue.pop();
	}
	goDeleteEnabled = false;
}

GameObject* Game::Instantiate(vec3 pos, vec3 rot, vec3 scale)
{
	GameObject* go = nullptr;
	if (gameObjects.size() < maxObjects)
	{
		tbb::spin_mutex::scoped_lock spinLock(addLock);
		go = new GameObject(pos, rot, scale);
		gameObjects[go->GetUID()] = go;
		addQueue.emplace(go);
	}
	return go;
}

const Terrain* Game::GetTerrain(vec3 pos) const
{
	return &terrains[0];
}

float Game::GetTime() const
{
	return static_cast<float>(glfwGetTime());
}

void Game::RemoveGameObject(GameObject* go)
{
	// TODO: need to make sure this won't get deadlocked
	tbb::spin_mutex::scoped_lock spinLock(removeLock);
	removeQueue.push(go);
}

void Game::LogToFile(string s)
{
	if (file.is_open())
		file << s << endl;
}