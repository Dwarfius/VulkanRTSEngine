#include "Precomp.h"
#include "Game.h"

#include "Input.h"
#include "Audio.h"
#include "Terrain.h"
#include "Terrain.h"
#include "VisualObject.h"
#include "Components/AnimationTest.h"
#include "Components/EditorMode.h"
#include "Components/PhysicsComponent.h"
#include "Graphics/Adapters/CameraAdapter.h"
#include "Graphics/Adapters/ObjectMatricesAdapter.h"
#include "Graphics/Adapters/TerrainAdapter.h"
#include "Graphics/Adapters/SkeletonAdapter.h"
#include "Systems/ImGUI/ImGUISystem.h"
#include "Animation/AnimationSystem.h"
#include "RenderThread.h"

#include <Core/StaticString.h>
#include <Core/Profiler.h>
#include <Core/Resources/AssetTracker.h>

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

#include <memory_resource>

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
{
	Profiler::ScopedMark profile("Game::Ctor");
	ourInstance = this;
	UID::Init();

	glfwSetErrorCallback(aReporterFunc);
	glfwInit();

	glfwSetTime(0);
	
	//Audio::Init();
	//Audio::SetMusicTrack(2);

	myAssetTracker = new AssetTracker();
	myImGUISystem = new ImGUISystem(*this);
	myAnimationSystem = new AnimationSystem();

	{
		constexpr float kTerrSize = 18000; // meters
		constexpr float kResolution = 928; // pixels
		Terrain* terr = new Terrain();
		// Heightmaps generated via https://tangrams.github.io/heightmapper/
		Handle<Texture> terrainText = myAssetTracker->GetOrCreate<Texture>("Tynemouth-tangrams.desc");
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
	delete myAssetTracker;
	delete myAnimationSystem;
	delete myImGUISystem;

	// render thread has to go first, since it relies on glfw to be there
	delete myRenderThread;
	glfwTerminate();
}

AssetTracker& Game::GetAssetTracker() 
{ 
	return *myAssetTracker; 
}

ImGUISystem& Game::GetImGUISystem() 
{ 
	return *myImGUISystem; 
}

AnimationSystem& Game::GetAnimationSystem() 
{ 
	return *myAnimationSystem; 
}

void Game::Init()
{
	Profiler::ScopedMark profile("Game::Init");
	RegisterUniformAdapters();

	constexpr static uint32_t kInitReserve = 4000;
	myGameObjects.reserve(kInitReserve);

	myRenderThread->Init(BootWithVK, *myAssetTracker);

	// TODO: has implicit dependency on window initialized - make explicit!
	myImGUISystem->Init();

	Handle<GameObject> go; 
	VisualObject* vo;

	// ==========================
	Handle<Pipeline> terrainPipeline = myAssetTracker->GetOrCreate<Pipeline>("terrain.ppl");

	// TODO: replace heap allocation with using 
	// a continuous allocated storage of VisualObjects

	// terrain
	go = new GameObject(Transform{});
	{
		vo = new VisualObject(*go.Get());
		vo->SetPipeline(terrainPipeline);
		vo->SetTexture(myTerrains[0]->GetTextureHandle());
		vo->SetCategory(VisualObject::Category::Terrain);
		go->SetVisualObject(vo);
	}
	myTerrains[0]->AddPhysicsEntity(*go.Get(), *myPhysWorld);
	AddGameObject(go);

	myEditorMode = new EditorMode(*this);
	myAnimTest = new AnimationTest(*this);

	// setting up a task tree
	{
		Profiler::ScopedMark profile("Game::SetupTaskTree");
		GameTask task(GameTask::UpdateInput, [this]() { UpdateInput(); });
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::AddGameObjects, [this]() { AddGameObjects(); });
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::PhysicsUpdate, [this]() { PhysicsUpdate(); });
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::AnimationUpdate, [this]() { AnimationUpdate(); });
		task.AddDependency(GameTask::PhysicsUpdate);
		myTaskManager->AddTask(task);

		task = GameTask(GameTask::EditorUpdate, [this]() { EditorUpdate(); });
		task.AddDependency(GameTask::UpdateInput);
		task.AddDependency(GameTask::PhysicsUpdate);
		task.AddDependency(GameTask::AnimationUpdate);
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

	myImGUISystem->Shutdown();

	delete myEditorMode;
	delete myAnimTest;

	// physics clear
	delete myPhysWorld;

	// we can mark that the engine is done - wrap the threads
	myIsRunning = false;
	// get rid of pending objects
	while (myAddQueue.size())
	{
		myAddQueue.pop();
	}

	// get rid of tracked objects
	for (auto [key, goHandle] : myGameObjects)
	{
		RemoveGameObject(goHandle);
	}
	RemoveGameObjects();
	ASSERT_STR(myGameObjects.empty(), "All objects should've been cleaned up!");

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

const Terrain* Game::GetTerrain(glm::vec3 pos) const
{
	return myTerrains[0];
}

float Game::GetTime() const
{
	return static_cast<float>(glfwGetTime());
}

Graphics* Game::GetGraphics() 
{ 
	return myRenderThread->GetGraphics(); 
}

const Graphics* Game::GetGraphics() const
{
	return myRenderThread->GetGraphics();
}

void Game::AddGameObject(Handle<GameObject> aGOHandle)
{
	ASSERT_STR(aGOHandle.IsValid(), "Invalid object passed in!");

	{
		tbb::spin_mutex::scoped_lock spinLock(myAddLock);
		myAddQueue.push(aGOHandle);
	}

	GameObject* go = aGOHandle.Get();
	const size_t childCount = go->GetChildCount();
	if (!childCount)
	{
		return;
	}

	// if there's children - schedule all of them for addition
	constexpr size_t kGOMemorySize = 128;
	GameObject* childMemory[kGOMemorySize];
	std::pmr::monotonic_buffer_resource stackRes(childMemory, sizeof(childMemory));
	std::pmr::vector<GameObject*> dirtyGOs(&stackRes);
	for (size_t childInd = 0; childInd < childCount; childInd++)
	{
		dirtyGOs.push_back(&go->GetChild(childInd));
	}

	while (dirtyGOs.size())
	{
		GameObject* dirtyGO = dirtyGOs.back();
		dirtyGOs.pop_back();

		{
			tbb::spin_mutex::scoped_lock spinLock(myAddLock);
			myAddQueue.push(dirtyGO);
		}

		const size_t dirtyCount = dirtyGO->GetChildCount();
		for (size_t childInd = 0; childInd < dirtyCount; childInd++)
		{
			dirtyGOs.push_back(&dirtyGO->GetChild(childInd));
		}
	}

}

void Game::RemoveGameObject(Handle<GameObject> aGOHandle)
{
	ASSERT_STR(aGOHandle.IsValid(), "Invalid object passed in!");

	{
		tbb::spin_mutex::scoped_lock spinLock(myRemoveLock);
		myRemoveQueue.push(aGOHandle);
	}

	// if there's a parent - let it stay in the world
	GameObject* go = aGOHandle.Get();
	if (go->GetParent().IsValid())
	{
		go->DetachFromParent();
	}

	const size_t childCount = go->GetChildCount();
	if (!childCount)
	{
		return;
	}

	// if there's children - schedule all of them for removal
	constexpr size_t kGOMemorySize = 128;
	GameObject* childMemory[kGOMemorySize];
	std::pmr::monotonic_buffer_resource stackRes(childMemory, sizeof(childMemory));
	std::pmr::vector<GameObject*> dirtyGOs(&stackRes);
	for (size_t childInd = 0; childInd < childCount; childInd++)
	{
		dirtyGOs.push_back(&go->GetChild(childInd));
	}

	while (dirtyGOs.size())
	{
		GameObject* dirtyGO = dirtyGOs.back();
		dirtyGOs.pop_back();

		{
			tbb::spin_mutex::scoped_lock spinLock(myRemoveLock);
			myRemoveQueue.push(dirtyGO);
		}

		// We need to detach from parent to break the circular dependency
		// between Parent <=> Child (because both of them are Handles)
		dirtyGO->DetachFromParent();

		const size_t dirtyCount = dirtyGO->GetChildCount();
		for (size_t childInd = 0; childInd < dirtyCount; childInd++)
		{
			dirtyGOs.push_back(&dirtyGO->GetChild(childInd));
		}
	}
}

void Game::AddGameObjects()
{
	// TODO: find a better place for this call
	myDebugDrawer.BeginFrame();

	tbb::spin_mutex::scoped_lock spinlock(myAddLock);
	while (myAddQueue.size())
	{
		Handle<GameObject> go = myAddQueue.front();
		myAddQueue.pop();
		myGameObjects[go->GetUID()] = go;
	}
}

void Game::UpdateInput()
{
	Profiler::ScopedMark profile(__func__);
	Input::Update();
	myImGUISystem->NewFrame(myDeltaTime);
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
	for (std::pair<const UID, Handle<GameObject>>& pair : myGameObjects)
	{
		pair.second->Update(myDeltaTime);
	}
}

void Game::EditorUpdate()
{
	Profiler::ScopedMark profile(__func__);
	ASSERT(myEditorMode);
	myEditorMode->Update(*this, myDeltaTime, *myPhysWorld);
	myAnimTest->Update(myDeltaTime);
}

void Game::PhysicsUpdate()
{
	if (!myIsPaused)
	{
		Profiler::ScopedMark profile(__func__);
		myPhysWorld->Simulate(myDeltaTime);
	}
}

void Game::AnimationUpdate()
{
	if (!myIsPaused)
	{
		Profiler::ScopedMark profile(__func__);
		myAnimationSystem->Update(myDeltaTime);
	}
}

void Game::Render()
{
	Profiler::ScopedMark profile(__func__);
	// TODO: get rid of single map, and use a separate vector for Renderables
	// TODO: fill out the renderables vector not per frame, but after new ones are created
	for (std::pair<const UID, Handle<GameObject>>& elem : myGameObjects)
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

	myImGUISystem->Render();

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
		Handle<GameObject> go = myRemoveQueue.front();
		myGameObjects.erase(go->GetUID());
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
	adapterRegister.Register<SkeletonAdapter>();
}