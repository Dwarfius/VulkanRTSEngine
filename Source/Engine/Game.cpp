#include "Precomp.h"
#include "Game.h"
#include "Graphics/Graphics.h"
#include "Graphics/UniformAdapterRegister.h"
#include "Input.h"
#include "Audio.h"
#include "Camera.h"
#include "Terrain.h"
#include "GameObject.h"
#include "Terrain.h"

#include <PhysicsWorld.h>
#include <PhysicsEntity.h>
#include <PhysicsShapes.h>

#include "VisualObject.h"
#include "Components/EditorMode.h"
#include "Components/PhysicsComponent.h"

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
	//myFile.open(BootWithVK ? "logVK.csv" : "logGL.csv");
	if (!myFile.is_open())
	{
		printf("[Warning] Log file didn't open\n");
	}

	//Audio::Init();
	//Audio::SetMusicTrack(2);

	// Trigger initialization
	UniformAdapterRegister::GetInstance();

	Terrain* terr = new Terrain();
	terr->Load(myAssetTracker, "assets/textures/heightmapSmall.png", 1.f, 1.f, 1.f);
	myTerrains.push_back(terr);

	myRenderThread = new RenderThread();

	myCamera = new Camera(Graphics::GetWidth(), Graphics::GetHeight());

	myTaskManager = make_unique<GameTaskManager>();

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
	// TODO: move this info to .ppl file
	Handle<Pipeline> defPipeline = myAssetTracker.Create<Pipeline>();
	Handle<Shader> defVert = myAssetTracker.GetOrCreate<Shader>("assets/GLShaders/base.vert");
	Handle<Shader> defFrag = myAssetTracker.GetOrCreate<Shader>("assets/GLShaders/base.frag");
	defPipeline->AddShader(defVert);
	defPipeline->AddShader(defFrag);
	defPipeline->SetState(Resource::State::PendingUpload);
	// ==========================
	Handle<Texture> cubeText = myAssetTracker.GetOrCreate<Texture>("CubeUnwrap.jpg");
	Handle<Texture> terrainText = myAssetTracker.GetOrCreate<Texture>("wireframe.png");

	// a box for rendering test
	go = Instantiate();
	{
		vo = new VisualObject(*go);
		vo->SetModel(cubeModel);
		vo->SetPipeline(defPipeline);
		vo->SetTexture(cubeText);
		go->SetVisualObject(vo);
	}
	go->GetTransform().SetPos(glm::vec3(0, 3, 0));
	go->SetCollisionsEnabled(false);

	// terrain
	go = Instantiate();
	{
		vo = new VisualObject(*go);
		vo->SetModel(myTerrains[0]->GetModelHandle());
		vo->SetPipeline(defPipeline);
		vo->SetTexture(terrainText);
		go->SetVisualObject(vo);
	}
	go->SetCollisionsEnabled(false);

	PhysicsComponent* physComp = new PhysicsComponent();
	physComp->SetPhysicsEntity(myTerrains[0]->CreatePhysics());
	go->AddComponent(physComp);
	physComp->RequestAddToWorld(*myPhysWorld);

	// a sphere for testing
	mySphereShape = make_shared<PhysicsShapeSphere>(1.f);
	myBall = make_shared<PhysicsEntity>(1.f, mySphereShape, glm::mat4(1.f));

	// player
	go = Instantiate();
	go->AddComponent(new EditorMode());

	// setting up a task tree
	{
		GameTask task(GameTask::UpdateInput, bind(&Game::UpdateInput, this));
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::AddGameObjects, bind(&Game::AddGameObjects, this));
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::PhysicsUpdate, bind(&Game::PhysicsUpdate, this));
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::GameUpdate, bind(&Game::Update, this));
		task.AddDependency(GameTask::UpdateInput);
		task.AddDependency(GameTask::AddGameObjects);
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

	// physics clear
	myBall.reset();
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
	/*{
		const float newTime = static_cast<float>(glfwGetTime());
		myDeltaTime = newTime - myFrameStart;
		myFrameStart = newTime;
	}*/

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

	// TODO: at the moment all gameobjects don't have cross-synchronization, so will need to fix this up
	for (const pair<UID, GameObject*>& pair : myGameObjects)
	{
		pair.second->Update(myDeltaTime);
	}

	// experiment #2 - dynamic world addition-removal from different thread
	if (myBall->GetState() == PhysicsEntity::NotInWorld)
	{
		// spawn the ball at the top
		glm::mat4 transform = glm::translate(glm::vec3(0.f, 5.f, 0.f));
		myBall->SetTransform(transform);
		myBall->SetVelocity(glm::vec3());
		myPhysWorld->AddEntity(myBall);
	}
	else if(myBall->GetState() == PhysicsEntity::InWorld && myBall->GetTransform()[3][1] < -5.f)
	{
		// despawn the ball
		myPhysWorld->RemoveEntity(myBall);
	}
}

void Game::PhysicsUpdate()
{
	myPhysWorld->Simulate(myDeltaTime);
}

void Game::Render()
{
	// TODO: get rid of single map, and use a separate vector for Renderables
	// TODO: fill out the renderables vector not per frame, but after new ones are created
	for (const pair<UID, GameObject*>& elem : myGameObjects)
	{
		const VisualObject* visObj = elem.second->GetVisualObject();
		if (visObj && visObj->IsValid())
		{
			myRenderThread->AddRenderable(visObj);
		}
	}

	// adding axis for world navigation
	myRenderThread->AddLine(glm::vec3(-10.f, 0.f, 0.f), glm::vec3(10.f, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));
	myRenderThread->AddLine(glm::vec3(0.f, -10.f, 0.f), glm::vec3(0.f, 10.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	myRenderThread->AddLine(glm::vec3(0.f, 0.f, -10.f), glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f, 0.f, 1.f));
	myRenderThread->AddLines(myPhysWorld->GetDebugLineCache());

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

void Game::LogToFile(string s)
{
	if (myFile.is_open())
	{
		myFile << s << endl;
	}
}