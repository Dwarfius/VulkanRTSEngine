#include "Common.h"
#include "Game.h"
#include "Graphics.h"
#include "Input.h"
#include "Audio.h"
#include "Camera.h"
#include "Terrain.h"
#include "GameObject.h"
#include "Terrain.h"

#include <PhysicsWorld.h>
#include <PhysicsEntity.h>
#include <PhysicsShapes.h>

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

Game* Game::ourInstance = nullptr;
bool Game::ourGODeleteEnabled = false;

constexpr bool BootWithVK = false;

Game::Game(ReportError aReporterFunc)
	: myFrameStart(0.f)
	, myDeltaTime(0.f)
	, myCamera(nullptr)
	, myIsRunning(true)
	, myShouldEnd(false)
	, myIsPaused(false)
{
	ourInstance = this;
	UID::Init();

	glfwSetErrorCallback(aReporterFunc);
	glfwInit();

	glfwSetTime(0);
	
	// TODO: Need to refactor out logging
	myFile.open(BootWithVK ? "logVK.csv" : "logGL.csv");
	if (!myFile.is_open())
	{
		printf("[Warning] Log file didn't open\n");
	}

	//Audio::Init();
	//Audio::SetMusicTrack(2);

	Terrain* terr = new Terrain();
	terr->Generate("assets/textures/heightmapSmall.png", 1.f, 1.f, 1.f);
	myTerrains.push_back(terr);

	myRenderThread = make_unique<RenderThread>();

	myCamera = new Camera(Graphics::GetWidth(), Graphics::GetHeight());

	myTaskManager = make_unique<GameTaskManager>();

	myPhysWorld = new PhysicsWorld();

	// TODO: need to clean this up and refactor
	// here goes the experiment
	terr->CreatePhysics();
	myPhysWorld->AddEntity(terr->GetPhysicsEntity());
	// a sphere for testing
	mySphereShape = new PhysicsShapeSphere(1.f);
	glm::mat4 transform = glm::translate(glm::vec3(0.f, 10.f, 0.f));
	myBall1 = new PhysicsEntity(1.f, *mySphereShape, transform);
	myPhysWorld->AddEntity(myBall1);
}

Game::~Game()
{
	glfwTerminate();
}

void Game::Init()
{
	myGameObjects.reserve(maxObjects);

	myRenderThread->Init(BootWithVK, &myTerrains);

	GameObject *go; 

	// terrain
	go = Instantiate();
	go->AddComponent(new Renderer(1, 0, 3));
	go->SetCollisionsEnabled(false);

	// player
	go = Instantiate();
	go->AddComponent(new EditorMode());

	GameTask task(GameTask::UpdateInput, bind(&Game::UpdateInput, this));
	myTaskManager->AddTask(task);

	task = GameTask(GameTask::AddGameObjects, bind(&Game::AddGameObjects, this));
	myTaskManager->AddTask(task);

	task = GameTask(GameTask::GameUpdate, bind(&Game::Update, this));
	task.AddDependency(GameTask::UpdateInput);
	task.AddDependency(GameTask::AddGameObjects);
	myTaskManager->AddTask(task);

	task = GameTask(GameTask::PhysicsUpdate, bind(&Game::PhysicsUpdate, this));
	myTaskManager->AddTask(task);

	task = GameTask(GameTask::RemoveGameObjects, bind(&Game::RemoveGameObjects, this));
	task.AddDependency(GameTask::GameUpdate);
	myTaskManager->AddTask(task);

	task = GameTask(GameTask::UpdateEnd, bind(&Game::UpdateEnd, this));
	task.AddDependency(GameTask::RemoveGameObjects);
	task.AddDependency(GameTask::PhysicsUpdate);
	myTaskManager->AddTask(task);

	task = GameTask(GameTask::Render, bind(&Game::Render, this));
	task.AddDependency(GameTask::UpdateEnd);
	myTaskManager->AddTask(task);

	// TODO: will need to fix up audio
	task = GameTask(GameTask::UpdateAudio, bind(&Game::UpdateAudio, this));
	task.AddDependency(GameTask::UpdateEnd);
	myTaskManager->AddTask(task);

	myTaskManager->ResolveDependencies();
	myTaskManager->Run();
}

void Game::RunMainThread()
{
	glfwPollEvents();

	if (myRenderThread->IsBusy())
	{
		myRenderThread->SubmitRenderables();

		// TODO: need a semaphore for this, to disconnect from the render thread
		RunTaskGraph();
	}
}

void Game::RunTaskGraph()
{
	myTaskManager->Run();
}

void Game::CleanUp()
{
	if (myFile.is_open())
	{
		myFile.close();
	}

	// physics clear
	delete myPhysWorld;
	delete myBall1;
	delete mySphereShape;

	// we can mark that the engine is done - wrap the threads
	myIsRunning = false;
	ourGODeleteEnabled = true;
	for (auto pair : myGameObjects)
	{
		delete pair.second;
	}
	myGameObjects.clear();

	for (Terrain* terrain : myTerrains)
	{
		delete terrain;
	}
	myTerrains.clear();

	delete myCamera;
}

bool Game::IsRunning() const
{
	return !glfwWindowShouldClose(myRenderThread->GetWindow()) && myIsRunning;
}

GLFWwindow* Game::GetWindow() const
{
	return myRenderThread->GetWindow();
}

void Game::AddGameObjects()
{
	tbb::spin_mutex::scoped_lock spinlock(myAddLock);
	while (myAddQueue.size())
	{
		GameObject* go = myAddQueue.front();
		myAddQueue.pop();
		myGameObjects[go->GetUID()] = go;
	}
}

void Game::UpdateInput()
{
	Input::Update();
}

void Game::Update()
{
	{
		const float newTime = static_cast<float>(glfwGetTime());
		myDeltaTime = newTime - myFrameStart;
		myFrameStart = newTime;
	}

	if (Input::GetKey(27) || myShouldEnd)
	{
		myIsPaused = myIsRunning = false;
		return;
	}

	if (Input::GetKeyPressed('B'))
	{
		myIsPaused = !myIsPaused;
	}

	if (myIsPaused)
	{
		return;
	}

	// TODO: at the moment all gameobjects don't have cross-synchronization, so will need to fix this up
	for (auto pair : myGameObjects)
	{
		pair.second->Update(myDeltaTime);
	}
}

void Game::PhysicsUpdate()
{
	myPhysWorld->Simulate(myDeltaTime);
}

void Game::Render()
{
	if (Input::GetKeyPressed('G'))
	{
		myRenderThread->RequestSwitch();
	}

	for (const pair<UID, GameObject*>& elem : myGameObjects)
	{
		if (elem.second->GetRenderer())
		{
			myRenderThread->AddRenderable(elem.second);
		}
	}

	// adding axis for world navigation
	myRenderThread->AddLine(glm::vec3(-10.f, 0.f, 0.f), glm::vec3(10.f, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));
	myRenderThread->AddLine(glm::vec3(0.f, -10.f, 0.f), glm::vec3(0.f, 10.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	myRenderThread->AddLine(glm::vec3(0.f, 0.f, -10.f), glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f, 0.f, 1.f));
	myRenderThread->AddLines(myPhysWorld->GetDebugLineCache());

	// we have to wait until the render thread finishes processing the submitted commands
	// otherwise we'll screw the command buffers
	while (myRenderThread->IsBusy())
	{
		tbb::this_tbb_thread::yield();
	}

	// signal that we have submitted extra work
	myRenderThread->Work();
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
	ourGODeleteEnabled = true;
	tbb::spin_mutex::scoped_lock spinlock(myRemoveLock);
	while (myRemoveQueue.size())
	{
		GameObject* go = myRemoveQueue.front();
		myGameObjects.erase(go->GetUID());
		delete go;
		myRemoveQueue.pop();
	}
	ourGODeleteEnabled = false;
}

GameObject* Game::Instantiate(glm::vec3 aPos /*=glm::vec3()*/, glm::vec3 aRot /*=glm::vec3()*/, glm::vec3 aScale /*=glm::vec3(1)*/)
{
	GameObject* go = nullptr;
	tbb::spin_mutex::scoped_lock spinlock(myAddLock);
	if (myGameObjects.size() < maxObjects)
	{
		go = new GameObject(aPos, aRot, aScale);
		myAddQueue.emplace(go);
	}
	return go;
}

const Terrain* Game::GetTerrain(glm::vec3 pos) const
{
	return myTerrains[0];
}

float Game::GetTime() const
{
	return static_cast<float>(glfwGetTime());
}

void Game::RemoveGameObject(GameObject* go)
{
	tbb::spin_mutex::scoped_lock spinLock(myRemoveLock);
	myRemoveQueue.push(go);
}

void Game::LogToFile(string s)
{
	if (myFile.is_open())
	{
		myFile << s << endl;
	}
}