#include "Precomp.h"
#include "Game.h"

#include "Input.h"
#include "Audio.h"
#include "Terrain.h"
#include "GameObject.h"
#include "Terrain.h"
#include "VisualObject.h"
#include "Components/EditorMode.h"
#include "Components/PhysicsComponent.h"
#include "Graphics/Adapters/CameraAdapter.h"
#include "Graphics/Adapters/ObjectMatricesAdapter.h"
#include "Graphics/Adapters/TerrainAdapter.h"

#include <Core/StaticString.h>
#include <Core/Profiler.h>

#include <Graphics/Camera.h>
#include <Graphics/Graphics.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/UniformAdapterRegister.h>

#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsShapes.h>

#include <Core/Pool.h>

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
	, myIsInFocus(false)
	, myEditorMode(nullptr)
	, myImGUISystem(*this)
{
	Profiler::ScopedMark profile("Game::Ctor");
	ourInstance = this;
	UID::Init();

	TestPool();

	glfwSetErrorCallback(aReporterFunc);
	glfwInit();

	glfwSetTime(0);
	
	//Audio::Init();
	//Audio::SetMusicTrack(2);

	{
		constexpr float kTerrSize = 18000; // meters
		constexpr float kResolution = 928; // pixels
		Terrain* terr = new Terrain();
		// Heightmaps generated via https://tangrams.github.io/heightmapper/
		Handle<Texture> terrainText = myAssetTracker.GetOrCreate<Texture>("Tynemouth-tangrams.desc");
		terr->Load(terrainText, kTerrSize / kResolution, 1000.f);
		constexpr uint32_t kTerrCells = 64;
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
	Profiler::ScopedMark profile("Game::Init");
	RegisterUniformAdapters();

	myGameObjects.reserve(kMaxObjects);

	myRenderThread->Init(BootWithVK, myAssetTracker);

	// TODO: has implicit dependency on window initialized - make explicit!
	myImGUISystem.Init();

	GameObject* go; 
	VisualObject* vo;

	Handle<Model> cubeModel = myAssetTracker.GetOrCreate<Model>("cube.obj");
	// ==========================
	Handle<Pipeline> defPipeline = myAssetTracker.GetOrCreate<Pipeline>("default.ppl");
	Handle<Texture> cubeText = myAssetTracker.GetOrCreate<Texture>("CubeUnwrap.jpg");
	Handle<Pipeline> terrainPipeline = myAssetTracker.GetOrCreate<Pipeline>("terrain.ppl");

	// TODO: replace heap allocation with using 
	// a continuous allocated storage of VisualObjects

	// terrain
	go = Instantiate();
	{
		vo = new VisualObject(*go);
		vo->SetPipeline(terrainPipeline);
		vo->SetTexture(myTerrains[0]->GetTextureHandle());
		vo->SetCategory(VisualObject::Category::Terrain);
		go->SetVisualObject(vo);
	}
	myTerrains[0]->AddPhysicsEntity(*go, *myPhysWorld);

	myEditorMode = new EditorMode(*myPhysWorld);

	// setting up a task tree
	{
		Profiler::ScopedMark profile("Game::SetupTaskTree");
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
	}

	{
		Profiler::ScopedMark profile("Game::ResolveTaskTreeAndRun");
		// TODO: need to add functionality to draw out the task tree ingame
		myTaskManager->ResolveDependencies();
		myTaskManager->Run();
		myTaskManager->Wait();
	}
}

void Game::RunMainThread()
{
	Profiler::GetInstance().NewFrame();
	Profiler::ScopedMark mainProfile(__func__);
	glfwPollEvents();

	// TODO: refactor to get rid of this requirement to avoid triggering
	// the entire task manager just to get the first frame
	if (myRenderThread->HasWork())
	{
		const float newTime = static_cast<float>(glfwGetTime());
		myDeltaTime = newTime - myFrameStart;
		myFrameStart = newTime;
		myIsInFocus = glfwGetWindowAttrib(GetWindow(), GLFW_FOCUSED) != 0;

		{
			GameTaskManager::ExternalDependencyScope dependency = myTaskManager->AddExternalDependency(GameTask::Type::Render);
			myTaskManager->Run();

			Profiler::ScopedMark renderablesProfile("Game::SubmitRenderables");
			myRenderThread->SubmitRenderables();
		}
		myTaskManager->Wait();
	}
}

void Game::CleanUp()
{
	// First, wait until last frame is done
	myTaskManager->Wait();
	while (myRenderThread->HasWork())
	{
		myRenderThread->SubmitRenderables();
	}

	myImGUISystem.Shutdown();

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

GameObject* Game::Instantiate(glm::vec3 aPos /*=glm::vec3()*/, glm::vec3 aRot /*=glm::vec3()*/, glm::vec3 aScale /*=glm::vec3(1)*/)
{
	Profiler::ScopedMark profile(__func__);
	GameObject* go = nullptr;
	tbb::spin_mutex::scoped_lock spinlock(myAddLock);
	if (myGameObjects.size() < kMaxObjects)
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
	Profiler::ScopedMark profile(__func__);
	Input::Update();
	myImGUISystem.NewFrame(myDeltaTime);
}

void Game::Update()
{
	Profiler::ScopedMark profile(__func__);
	if (Input::GetKey(Input::Keys::Escape) || myShouldEnd)
	{
		myIsPaused = myIsRunning = false;
		return;
	}

	if (Input::GetKeyPressed(Input::Keys::B))
	{
		myIsPaused = !myIsPaused;
	}

	if (Input::GetKeyPressed(Input::Keys::G))
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
	Profiler::ScopedMark profile(__func__);
	ASSERT(myEditorMode);
	myEditorMode->Update(*this, myDeltaTime, *myPhysWorld);
}

void Game::PhysicsUpdate()
{
	Profiler::ScopedMark profile(__func__);
	myPhysWorld->Simulate(myDeltaTime);
}

void Game::Render()
{
	Profiler::ScopedMark profile(__func__);
	// TODO: get rid of single map, and use a separate vector for Renderables
	// TODO: fill out the renderables vector not per frame, but after new ones are created
	for (const std::pair<UID, GameObject*>& elem : myGameObjects)
	{
		VisualObject* visObj = elem.second->GetVisualObject();
		if (visObj)
		{
			myRenderThread->AddRenderable(visObj);
		}
	}

	// adding axis for world navigation
	const Terrain* terrain = myTerrains[0];
	const float halfW = terrain->GetWidth() / 2.f;
	const float halfH = 2;
	const float halfD = terrain->GetDepth() / 2.f;
	
	myDebugDrawer.AddLine(glm::vec3(-halfW, 0.f, 0.f), glm::vec3(halfW, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));
	myDebugDrawer.AddLine(glm::vec3(0.f, -halfH, 0.f), glm::vec3(0.f, halfH, 0.f), glm::vec3(0.f, 1.f, 0.f));
	myDebugDrawer.AddLine(glm::vec3(0.f, 0.f, -halfD), glm::vec3(0.f, 0.f, halfD), glm::vec3(0.f, 0.f, 1.f));

	myRenderThread->AddDebugRenderable(&myDebugDrawer);
	myRenderThread->AddDebugRenderable(&myPhysWorld->GetDebugDrawer());

	myImGUISystem.Render();

	// signal that we have submitted extra work
	myRenderThread->Work();
}

void Game::UpdateAudio()
{
	Profiler::ScopedMark profile(__func__);
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
	Profiler::ScopedMark profile(__func__);
	Input::PostUpdate();
}

void Game::RemoveGameObjects()
{
	Profiler::ScopedMark profile(__func__);
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

void Game::RegisterUniformAdapters()
{
	UniformAdapterRegister& adapterRegister = UniformAdapterRegister::GetInstance();
	adapterRegister.Register<CameraAdapter>();
	adapterRegister.Register<ObjectMatricesAdapter>();
	adapterRegister.Register<TerrainAdapter>();
}

void Game::TestPool()
{
	Profiler::ScopedMark profile("Game::TestPool");
	constexpr static auto kSumTest = [](Pool<uint32_t>& aPool, size_t anExpectedSum) {
		int sum = 0;
		aPool.ForEach([&sum](const int& anItem) {
			sum += anItem;
		});
		ASSERT(sum == anExpectedSum);
	};

	Pool<uint32_t> poolOfInts;
	std::vector<Pool<uint32_t>::Ptr> intPtrs;
	// basic test
	intPtrs.emplace_back(std::move(poolOfInts.Allocate(0)));
	ASSERT(*intPtrs[0].Get() == 0);
	ASSERT(poolOfInts.GetSize() == 1);
	intPtrs.clear();
	ASSERT(poolOfInts.GetSize() == 0);

	size_t oldCapacity = poolOfInts.GetCapacity();
	// up to capacity+consistency test 
	size_t expectedSum = 0;
	for (uint32_t i = 0; i < poolOfInts.GetCapacity(); i++)
	{
		intPtrs.emplace_back(std::move(poolOfInts.Allocate(i)));
		expectedSum += i;
	}
	ASSERT(poolOfInts.GetSize() == poolOfInts.GetCapacity());
	ASSERT(poolOfInts.GetCapacity() == oldCapacity);
	kSumTest(poolOfInts, expectedSum);

	// capacity growth test
	uint32_t newElem = static_cast<uint32_t>(poolOfInts.GetCapacity());
	intPtrs.emplace_back(std::move(poolOfInts.Allocate(newElem)));
	expectedSum += newElem;
	ASSERT(poolOfInts.GetCapacity() > oldCapacity);
	kSumTest(poolOfInts, expectedSum);

	// partial clear test
	size_t removedElem = 10;
	intPtrs.erase(intPtrs.begin() + removedElem);
	expectedSum -= removedElem;
	kSumTest(poolOfInts, expectedSum);

	// weak ptr test
	{
		Pool<uint32_t>::WeakPtr weakPtr;
		weakPtr = intPtrs[0];
		ASSERT(*weakPtr.Get() == 0);
	}
	kSumTest(poolOfInts, expectedSum);

	// full clear test
	oldCapacity = poolOfInts.GetCapacity();
	intPtrs.clear();
	ASSERT(poolOfInts.GetSize() == 0);
	ASSERT(poolOfInts.GetCapacity() == oldCapacity);
	kSumTest(poolOfInts, 0);
}