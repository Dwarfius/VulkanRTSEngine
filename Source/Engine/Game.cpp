#include "Precomp.h"
#include "Game.h"

#include "Graphics/Adapters/UniformAdapterRegister.h"
#include "Input.h"
#include "Audio.h"
#include "Terrain.h"
#include "GameObject.h"
#include "Terrain.h"

#include <Core/StaticString.h>

#include <Graphics/Camera.h>
#include <Graphics/Graphics.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>

#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsShapes.h>

#include "VisualObject.h"
#include "Components/EditorMode.h"
#include "Components/PhysicsComponent.h"

// TODO: write own header for collision object flags
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

Game* Game::ourInstance = nullptr;
bool Game::ourGODeleteEnabled = false;

constexpr bool BootWithVK = false;


constexpr StaticString kHeightmapName("Tynemouth-tangrams.png");

Game::Game(ReportError aReporterFunc)
	: myFrameStart(0.f)
	, myDeltaTime(0.f)
	, myCamera(nullptr)
	, myIsRunning(true)
	, myShouldEnd(false)
	, myIsPaused(false)
	, myEditorMode(nullptr)
{
	ourInstance = this;
	UID::Init();

	glfwSetErrorCallback(aReporterFunc);
	glfwInit();

	glfwSetTime(0);
	
	// TODO: Need to refactor out logging
	//myFile.open(BootWithVK ? "logVK.csv" : "logGL.csv");
	if (!myFile.is_open())
	{
		printf("[Warning] Log file didn't open\n");
	}

	//Audio::Init();
	//Audio::SetMusicTrack(2);

	// Trigger initialization
	UniformAdapterRegister::GetInstance();

	{
		constexpr float kTerrSize = 18000; // meters
		constexpr float kResolution = 928; // pixels
		Terrain* terr = new Terrain();
		// Heightmaps generated via https://tangrams.github.io/heightmapper/
		Handle<Texture> terrainText = myAssetTracker.GetOrCreate<Texture>("Tynemouth-tangrams.desc");
		terr->Load(terrainText, kTerrSize / kResolution, 1000.f);
		constexpr uint32_t kTerrCells = 8;
		//terr->Generate(glm::ivec2(kTerrCells, kTerrCells), 1, 10);
		myTerrains.push_back(terr);
	}

	myRenderThread = new RenderThread();

	myCamera = new Camera(Graphics::GetWidth(), Graphics::GetHeight());

	myTaskManager = std::make_unique<GameTaskManager>();

	myPhysWorld = new PhysicsWorld();
}

Game::~Game()
{
	// render thread has to go first, since it relies on glfw to be there
	delete myRenderThread;
	glfwTerminate();
}

void Game::Init()
{
	myGameObjects.reserve(maxObjects);

	myRenderThread->Init(BootWithVK, myAssetTracker);

	GameObject* go; 
	VisualObject* vo;

	Handle<Model> cubeModel = myAssetTracker.GetOrCreate<Model>("cube.obj");
	// ==========================
	Handle<Pipeline> defPipeline = myAssetTracker.GetOrCreate<Pipeline>("default.ppl");
	Handle<Texture> cubeText = myAssetTracker.GetOrCreate<Texture>("CubeUnwrap.jpg");
	Handle<Pipeline> terrainPipeline = myAssetTracker.GetOrCreate<Pipeline>("terrain.ppl");

	// TODO: replace heap allocation with using 
	// a continuous allocated storage of VisualObjects

	// a box for rendering test
	/*go = Instantiate();
	{
		vo = new VisualObject(*go);
		vo->SetModel(cubeModel);
		vo->SetPipeline(defPipeline);
		vo->SetTexture(cubeText);
		go->SetVisualObject(vo);
	}
	go->GetTransform().SetPos(glm::vec3(0, 3, 0));*/

	// terrain
	go = Instantiate();
	{
		vo = new VisualObject(*go);
		vo->SetModel(myTerrains[0]->GetModelHandle());
		vo->SetPipeline(terrainPipeline);
		vo->SetTexture(myTerrains[0]->GetTextureHandle());
		vo->SetCategory(VisualObject::Category::Terrain);
		go->SetVisualObject(vo);
	}

	PhysicsComponent* physComp = go->AddComponent<PhysicsComponent>();
	physComp->CreatePhysicsEntity(0, myTerrains[0]->CreatePhysicsShape());
	physComp->GetPhysicsEntity().SetCollisionFlags(
		physComp->GetPhysicsEntity().GetCollisionFlags() 
		| btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT
	);
	physComp->RequestAddToWorld(*myPhysWorld);

	myEditorMode = new EditorMode(*myPhysWorld);

	// setting up a task tree
	{
		GameTask task(GameTask::UpdateInput, [this]() { UpdateInput(); });
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::AddGameObjects, [this]() { AddGameObjects(); });
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::PhysicsUpdate, [this]() { PhysicsUpdate(); });
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::EditorUpdate, [this]() { EditorUpdate(); });
		task.AddDependency(GameTask::UpdateInput);
		task.AddDependency(GameTask::PhysicsUpdate);
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::GameUpdate, [this]() { Update(); });
		task.AddDependency(GameTask::UpdateInput);
		task.AddDependency(GameTask::AddGameObjects);
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::RemoveGameObjects, [this]() { RemoveGameObjects(); });
		task.AddDependency(GameTask::GameUpdate);
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::UpdateEnd, [this]() { UpdateEnd(); });
		task.AddDependency(GameTask::RemoveGameObjects);
		task.AddDependency(GameTask::EditorUpdate);
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::Render, [this]() { Render(); });
		task.AddDependency(GameTask::UpdateEnd);
		myTaskManager->AddTask(task);

		// TODO: will need to fix up audio
		task = GameTask(GameTask::UpdateAudio, [this]() { UpdateAudio(); });
		task.AddDependency(GameTask::UpdateEnd);
		myTaskManager->AddTask(task);

		// TODO: need to add functionality to draw out the task tree ingame
		myTaskManager->ResolveDependencies();
		myTaskManager->Run();
	}
}

void Game::RunMainThread()
{
	glfwPollEvents();

	if (myRenderThread->HasWork())
	{
		const float newTime = static_cast<float>(glfwGetTime());
		myDeltaTime = newTime - myFrameStart;
		myFrameStart = newTime;

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
	// First, wait until render-thread is done
	while (myRenderThread->HasWork())
	{
		myRenderThread->SubmitRenderables();
	}

	// now that it's done, we can clean up everything
	if (myFile.is_open())
	{
		myFile.close();
	}

	delete myEditorMode;

	// physics clear
	delete myPhysWorld;

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
	// TODO: find a better place for this call
	myDebugDrawer.BeginFrame();

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
	if (Input::GetKey(27) || myShouldEnd)
	{
		myIsPaused = myIsRunning = false;
		return;
	}

	if (Input::GetKeyPressed('B'))
	{
		myIsPaused = !myIsPaused;
	}

	if (Input::GetKeyPressed('G'))
	{
		myRenderThread->RequestSwitch();
	}

	if (myIsPaused)
	{
		return;
	}

	// TODO: get rid of this and switch to ECS style synchronization via task
	// dependency management
	for (const std::pair<UID, GameObject*>& pair : myGameObjects)
	{
		pair.second->Update(myDeltaTime);
	}
}

void Game::EditorUpdate()
{
	ASSERT(myEditorMode);
	myEditorMode->Update(myDeltaTime, *myPhysWorld);
}

void Game::PhysicsUpdate()
{
	myPhysWorld->Simulate(myDeltaTime);
}

void Game::Render()
{
	// TODO: get rid of single map, and use a separate vector for Renderables
	// TODO: fill out the renderables vector not per frame, but after new ones are created
	for (const std::pair<UID, GameObject*>& elem : myGameObjects)
	{
		VisualObject* visObj = elem.second->GetVisualObject();
		if (visObj && visObj->IsValid())
		{
			myRenderThread->AddRenderable(visObj);
		}
	}

	// adding axis for world navigation
	constexpr float kLength = 1000;
	myDebugDrawer.AddLine(glm::vec3(-kLength, 0.f, 0.f), glm::vec3(kLength, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));
	myDebugDrawer.AddLine(glm::vec3(0.f, -kLength, 0.f), glm::vec3(0.f, kLength, 0.f), glm::vec3(0.f, 1.f, 0.f));
	myDebugDrawer.AddLine(glm::vec3(0.f, 0.f, -kLength), glm::vec3(0.f, 0.f, kLength), glm::vec3(0.f, 0.f, 1.f));

	myRenderThread->AddDebugRenderable(&myDebugDrawer);
	myRenderThread->AddDebugRenderable(&myPhysWorld->GetDebugDrawer());

	// TODO: test moving it to the start of Render()
	// we have to wait until the render thread finishes processing the submitted commands
	// otherwise we'll screw the command buffers
	while (myRenderThread->HasWork())
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

void Game::LogToFile(const std::string& s)
{
	if (myFile.is_open())
	{
		myFile << s << std::endl;
	}
}