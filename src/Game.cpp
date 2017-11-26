#include "Common.h"
#include "Game.h"
#include "Grid.h"
#include "Graphics.h"
#include "Input.h"
#include "Audio.h"
#include "GameObject.h"
#include "Camera.h"
#include "Terrain.h"

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

Game* Game::inst = nullptr;

Game::Game()
{
	inst = this;
	for (size_t i = 0; i < Game::maxObjects; i++)
		ids.push(i);

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
	aliveGOs.reserve(maxObjects);

	taskManager = make_unique<GameTaskManager>();
}

void Game::Init()
{
	gameObjects.reserve(maxObjects);
	aliveGOs.reserve(maxObjects);
	
	renderThread->Init(isVK, &terrains);
	while (!renderThread->HasInitialised())
		this_thread::yield();

	GameObject *go; 

	// terrain
	go = Instantiate();
	go->AddComponent(new Renderer(1, 0, 3));
	go->SetCollisionsEnabled(false);

	// player
	go = Instantiate();
	go->AddComponent(new GameMode());

	GameTask task = GameTask(GameTask::UpdateInput, bind(&Game::UpdateInput, this));
	taskManager->AddTask(task);

	task = GameTask(GameTask::GameUpdate, bind(&Game::Update, this));
	task.AddDependency(GameTask::UpdateInput);
	taskManager->AddTask(task);

	// TODO: need to fix collisions
	task = GameTask(GameTask::CollisionUpdate, bind(&Game::CollisionUpdate, this));
	task.AddDependency(GameTask::GameUpdate);
	taskManager->AddTask(task);

	task = GameTask(GameTask::UpdateEnd, bind(&Game::UpdateEnd, this));
	task.AddDependency(GameTask::GameUpdate);
	task.AddDependency(GameTask::CollisionUpdate);
	taskManager->AddTask(task);

	task = GameTask(GameTask::Render, bind(&Game::Render, this));
	task.AddDependency(GameTask::UpdateEnd);
	taskManager->AddTask(task);

	// TODO: will need to fix up audio
	task = GameTask(GameTask::UpdateAudio, bind(&Game::UpdateAudio, this));
	task.AddDependency(GameTask::UpdateEnd);
	taskManager->AddTask(task);

	// TODO: finish this task
	task = GameTask(GameTask::RemoveGameObjects, bind(&Game::RemoveGameObjects, this));
	task.AddDependency(GameTask::Render);
	taskManager->AddTask(task);

	taskManager->ResolveDependencies();
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
	for (size_t i = 0; i < threads.size(); i++)
		threads[i].join();

	for (GameObject *go : gameObjects)
		delete go;
	gameObjects.clear();

	delete camera;
	delete grid;
}

void Game::UpdateInput()
{
	frameStart = static_cast<float>(glfwGetTime());
	// TODO: fix up input update
	Input::Update();
}

void Game::Update()
{
	if (Input::GetKey(27) || shouldEnd)
	{
		paused = running = false;
		return;
	}

	if (Input::GetKeyPressed('B'))
		paused = !paused;
	if (paused)
		return;

	if (Input::GetKeyPressed('I'))
		sensitivity += 0.3f;
	if (Input::GetKeyPressed('K') && sensitivity >= 0.3f)
		sensitivity -= 0.3f;

	// TODO: at the moment all gameobjects don't have cross-synchronization, so will need to fix this up
	for (size_t i = 0, end = gameObjects.size(); i < end; i++)
	{
		GameObject *go = gameObjects[i];
		go->Update(deltaTime);
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
		
		for (size_t i = 0, end = gameObjects.size(); i < end; i++)
			grid->Add(gameObjects[i], 0);

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
	// we have to wait until the render thread finishes processing the submitted commands
	// otherwise we'll overflow the command buffers
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
	deltaTime = static_cast<float>(glfwGetTime()) - frameStart;
}

void Game::RemoveGameObjects()
{
	// this is the place to clear out our gameobjects
	// the render calls that have been scheduled have been consumed
	// so no extra references are left
	// can't use parallel_for because it seems to break down the rendering loop
	// because there's no way that I know of atm to wait until the parallel_for finishes
	for (GameObject *go : gameObjects)
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
}

GameObject* Game::Instantiate(vec3 pos, vec3 rot, vec3 scale)
{
	if (gameObjects.size() == maxObjects - 1)
		return nullptr;

	GameObject *go = new GameObject(pos, rot, scale);
	go->SetIndex(ClaimId());
	gameObjects.push_back(go);

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

void Game::LogToFile(string s)
{
	if (file.is_open())
		file << s << endl;
}

void Game::ReturnId(size_t id)
{
	ids.push(id);
}

size_t Game::ClaimId()
{
	size_t id;
	if (!ids.try_pop(id))
		id = numeric_limits<size_t>::max();
	return id;
}

Graphics* Game::GetGraphicsRaw()
{
	return renderThread->GetGraphicsRaw();
}

const Graphics* Game::GetGraphics() const
{
	return renderThread->GetGraphics();
}