#include "Common.h"
#include "Game.h"
#include "Grid.h"
#include "Graphics.h"
#include "Input.h"
#include "Audio.h"
#include "Camera.h"
#include "Terrain.h"
#include "GameObject.h"
#include "Terrain.h"

#include <PhysicsWorld.h>

#include "Components\Renderer.h"
#include "Components\PlayerTank.h"
#include "Components\Tank.h"
#include "Components\EditorMode.h"

// prints out the thread states to track transitions
//#define DEBUG_THREADS

// should the engine put main thread to sleep instead of yielding it
// for 1 nanosecond during waiting for workers to finish
// yield leads to a smoother draw-rate, also more optimal for Vulkan judging by test
//#define USE_SLEEP

Game* Game::inst = nullptr;
bool Game::goDeleteEnabled = false;

Game::Game(ReportError reporterFunc)
{
	inst = this;
	UID::Init();

	glfwSetErrorCallback(reporterFunc);
	glfwInit();

	glfwSetTime(0);
	
	file.open(isVK ? "logVK.csv" : "logGL.csv");
	if (!file.is_open())
		std::printf("[Warning] Log file didn't open\n");

	//Audio::Init();
	//Audio::SetMusicTrack(2);

	Terrain* terr = new Terrain();
	terr->Generate("assets/textures/heightmap.png", 0.3f, glm::vec3(), 2.f, 1);
	terrains.push_back(terr);

	renderThread = make_unique<RenderThread>();

	camera = new Camera(Graphics::GetWidth(), Graphics::GetHeight());

	taskManager = make_unique<GameTaskManager>();

	myPhysWorld = new PhysicsWorld();
}

Game::~Game()
{
	glfwTerminate();
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
	go->AddComponent(new EditorMode());

	GameTask task(GameTask::UpdateInput, bind(&Game::UpdateInput, this));
	taskManager->AddTask(task);

	task = GameTask(GameTask::AddGameObjects, bind(&Game::AddGameObjects, this));
	taskManager->AddTask(task);

	task = GameTask(GameTask::GameUpdate, bind(&Game::Update, this));
	task.AddDependency(GameTask::UpdateInput);
	task.AddDependency(GameTask::AddGameObjects);
	taskManager->AddTask(task);

	// TODO: Need to restructure the graph to get rid of game update dependency
	// probably do AddGameObjects->PhysicsUpdate->GameUpdate
	task = GameTask(GameTask::PhysicsUpdate, bind(&Game::PhysicsUpdate, this));
	task.AddDependency(GameTask::GameUpdate);
	taskManager->AddTask(task);

	task = GameTask(GameTask::UpdateEnd, bind(&Game::UpdateEnd, this));
	task.AddDependency(GameTask::GameUpdate);
	task.AddDependency(GameTask::PhysicsUpdate);
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
	glfwPollEvents();

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
	{
		file.close();
	}

	// we can mark that the engine is done - wrap the threads
	running = false;
	goDeleteEnabled = true;
	for (auto pair : gameObjects)
	{
		delete pair.second;
	}
	gameObjects.clear();

	for (Terrain* terrain : terrains)
	{
		delete terrain;
	}
	terrains.clear();

	delete camera;

	delete myPhysWorld;
}

bool Game::IsRunning() const
{
	return !glfwWindowShouldClose(renderThread->GetWindow()) && running;
}

GLFWwindow* Game::GetWindow() const
{
	return renderThread->GetWindow();
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

void Game::PhysicsUpdate()
{
	// TODO: make it run at 30/s freq
	myPhysWorld->Simulate(deltaTime);
}

void Game::Render()
{
	if (Input::GetKeyPressed('G'))
	{
		renderThread->RequestSwitch();
	}

	for (const pair<UID, GameObject*>& elem : gameObjects)
	{
		renderThread->AddRenderable(elem.second);
	}

	// we have to wait until the render thread finishes processing the submitted commands
	// otherwise we'll screw the command buffers
	while (renderThread->IsBusy())
	{
		tbb::this_tbb_thread::yield();
	}

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

GameObject* Game::Instantiate(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
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

const Terrain* Game::GetTerrain(glm::vec3 pos) const
{
	return terrains[0];
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