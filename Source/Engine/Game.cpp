#include "Precomp.h"
#include "Game.h"

#include "Input.h"
#include "Audio.h"
#include "Terrain.h"
#include "Tests.h"
#include "VisualObject.h"
#include "Components/PhysicsComponent.h"
#include "Graphics/Adapters/CameraAdapter.h"
#include "Graphics/Adapters/ObjectMatricesAdapter.h"
#include "Graphics/RenderPasses/FinalCompositeRenderPass.h"
#include "Graphics/RenderPasses/DebugRenderPass.h"
#include "Systems/ImGUI/ImGUISystem.h"
#include "Systems/ImGUI/ImGUIRendering.h"
#include "Animation/AnimationSystem.h"
#include "Light.h"
#include "RenderThread.h"
#include "Resources/FileWatcher.h"
#include "UIWidgets/EntitiesWidget.h"
#include "UIWidgets/ObjImportDialog.h"
#include "UIWidgets/GltfImportDialog.h"
#include "UIWidgets/TerrainOptionsDialog.h"
#include "UIWidgets/TextureImportDialog.h"
#include "UIWidgets/ShaderCreateDialog.h"
#include "UIWidgets/PipelineCreateDialog.h"
#include "UIWidgets/GameTasksDialog.h"
#include "Systems/ProfilerUI.h"

#include <Core/Profiler.h>
#include <Core/Resources/AssetTracker.h>

#include <Graphics/Camera.h>
#include <Graphics/Graphics.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/UniformBuffer.h>
#include <Graphics/UniformAdapterRegister.h>

#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsShapes.h>

#include <Core/Pool.h>

#include <memory_resource>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

Game* Game::ourInstance = nullptr;
bool Game::ourGODeleteEnabled = false;

constexpr bool BootWithVK = false;

namespace UIWidgets
{
	void DrawCameraInfo(const Game& aGame, bool& aIsVisible)
	{
		if (ImGui::Begin("Camera", &aIsVisible))
		{
			const Camera* camera = aGame.GetCamera();
			const Transform& camTransform = camera->GetTransform();
			ImGui::Text("Pos: %f %f %f",
				camTransform.GetPos().x,
				camTransform.GetPos().y,
				camTransform.GetPos().z
			);
			ImGui::Text("Rot: %f %f %f",
				glm::degrees(camTransform.GetEuler().x),
				glm::degrees(camTransform.GetEuler().y),
				glm::degrees(camTransform.GetEuler().z)
			);
		}
		ImGui::End();
	}

	void DrawEngineSettings(Game& aGame, bool& aIsOpen)
	{
		if (ImGui::Begin("Engine Settings", &aIsOpen))
		{
			EngineSettings& settings = aGame.GetEngineSettings();
			ImGui::Checkbox("Is Paused (B)", &settings.myIsPaused);
			ImGui::Checkbox("Draw Wireframe (K)", &settings.myUseWireframe);
			ImGui::Checkbox("Draw Physics Debug", &settings.myDrawPhysicsDebug);
		}
		ImGui::End();
	}

	// We can't capture a copy in the lambda capture list because
	// it moves it around - and internally ProfielrUI
	// caches 'this' pointer (which becomes invalid)
	ProfilerUI ourProfilerUI;
	void DrawProfilerUI(bool& aIsVisible)
	{
		ourProfilerUI.Draw(aIsVisible);
	}
}

Game::Game(ReportError aReporterFunc)
	: myFrameStart(0.f)
	, myDeltaTime(0.f)
	, myCamera(nullptr)
	, myIsRunning(true)
	, myShouldEnd(false)
	, myIsInFocus(false)
{
	{
		Profiler::ScopedMark profile("Tests");
		Tests::RunTests();
	}

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
	myLightSystem = new LightSystem();

	myRenderThread = new RenderThread();

	myTaskManager = std::make_unique<GameTaskManager>();

	myPhysWorld = new PhysicsWorld();

	myFileWatcher = new FileWatcher(Resource::kAssetsFolder);
}

Game::~Game()
{
	delete myAnimationSystem;
	delete myImGUISystem;
	delete myLightSystem;

	// render thread has to go first, since it relies on glfw to be there
	delete myRenderThread;
	delete myAssetTracker;
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

LightSystem& Game::GetLightSystem()
{
	return *myLightSystem;
}

void Game::Init(bool aUseDefaultCompositePass)
{
	Profiler::ScopedMark profile("Game::Init");

	constexpr static uint32_t kInitReserve = 4000;
	myWorld.Reserve(kInitReserve);

	myRenderThread->Init(BootWithVK, *myAssetTracker);

	{
		Graphics* graphics = myRenderThread->GetGraphics();
		if (aUseDefaultCompositePass)
		{
			graphics->AddRenderPass(new FinalCompositeRenderPass(
				*graphics, myAssetTracker->GetOrCreate<Pipeline>("Engine/composite.ppl")
			));
			graphics->AddRenderPassDependency(FinalCompositeRenderPass::kId, ImGUIRenderPass::kId);
		}
		myCamera = new Camera(graphics->GetWidth(), graphics->GetHeight());
		myCamera->GetTransform().SetPos(glm::vec3(0, 0, 5));
		myCamera->GetTransform().LookAt(glm::vec3(0, 0, 0));
	}

	myImGUISystem->Init(*GetWindow());

	// setting up a task tree
	{
		GameTask task(Tasks::BeginFrame, [this] { BeginFrame(); });
		task.SetName("BeginFrame");
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::AddGameObjects, [this] { AddGameObjects(); });
		task.SetName("AddGameObjects");
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::PhysicsUpdate, [this] { PhysicsUpdate(); });
		task.SetName("PhysicsUpdate");
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::AnimationUpdate, [this] { AnimationUpdate(); });
		task.AddDependency(Tasks::PhysicsUpdate);
		task.SetName("AnimationUpdate");
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::GameUpdate, [this] { Update(); });
		task.AddDependency(Tasks::BeginFrame);
		task.AddDependency(Tasks::AddGameObjects);
		task.SetName("GameUpdate");
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::RemoveGameObjects, [this] { RemoveGameObjects(); });
		task.AddDependency(Tasks::GameUpdate);
		task.SetName("RemoveGameObjects");
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::UpdateEnd, [this] { UpdateEnd(); });
		task.AddDependency(Tasks::RemoveGameObjects);
		task.SetName("UpdateEnd");
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::Render, [this] { Render(); });
		task.AddDependency(Tasks::UpdateEnd);
		task.SetName("Render");
		myTaskManager->AddTask(task);

		// TODO: will need to fix up audio
		task = GameTask(Tasks::UpdateAudio, [this] { UpdateAudio(); });
		task.AddDependency(Tasks::UpdateEnd);
		task.SetName("UpdateAudio");
		myTaskManager->AddTask(task);
	}

	myTopBar.Register("Widgets/Demo", [](bool& aIsVisible) {
		ImGui::ShowDemoWindow(&aIsVisible);
	});
	myTopBar.Register("Widgets/Camera Info", [&](bool& aIsVisible) {
		UIWidgets::DrawCameraInfo(*this, aIsVisible);
	});
	myTopBar.Register("Widgets/Profiler", &UIWidgets::DrawProfilerUI);
	myTopBar.Register("Widgets/Game Tasks", &GameTasksDialog::Draw);
	myTopBar.Register("Widgets/Entities View",
		[&, entitiesWidget = EntitiesWidget()](bool& aIsVisible) mutable {
		entitiesWidget.DrawDialog(*this, aIsVisible);
	});
	myTopBar.Register("Widgets/Engine Settings", [&](bool& aIsVisible) {
		UIWidgets::DrawEngineSettings(*this, aIsVisible);
	});
	myTopBar.Register("Widgets/Terrain Options",
		[&, terrainOptions = TerrainOptionsDialog()](bool& aIsVisible) mutable {
		terrainOptions.Draw(*this, aIsVisible);
	});
	myTopBar.Register("File/Import OBJ",
		[objImportDialog = ObjImportDialog()](bool& aIsVisible) mutable {
		objImportDialog.Draw(aIsVisible);
	});
	myTopBar.Register("File/Import Gltf",
		[gltfImportDialog = GltfImportDialog()](bool& aIsVisible) mutable {
		gltfImportDialog.Draw(aIsVisible);
	});
	myTopBar.Register("File/Import Texture",
		[textureImportDialog = TextureImportDialog()](bool& aIsVisible) mutable {
		textureImportDialog.Draw(aIsVisible);
	});
	myTopBar.Register("File/Create Shader",
		[shaderCreateDialog = ShaderCreateDialog()](bool& aIsVisible) mutable {
		shaderCreateDialog.Draw(aIsVisible);
	});
	myTopBar.Register("File/Create Pipeline",
		[pipelineCreateDialog = PipelineCreateDialog()](bool& aIsVisible) mutable {
		pipelineCreateDialog.Draw(aIsVisible);
	});

	// TODO: need to add functionality to draw out the task tree ingame
	myTaskManager->Run();
	myTaskManager->Wait();
}

void Game::RunMainThread()
{
	Profiler::ScopedMark mainProfile(__func__);
	Profiler::GetInstance().NewFrame();
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
			Profiler::ScopedMark fileWatcherProfile("Game::CheckFiles");
			myFileWatcher->CheckFiles();
			for (const std::string& file : myFileWatcher->GetModifs())
			{
				std::println("FileWatcher: Detected change! {}", file);
				myRenderThread->GetGraphics()->FileChanged(file);
			}
		}

		{
			GameTaskManager::ExternalDependencyScope dependency = myTaskManager->AddExternalDependency(Tasks::Render);
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

	delete myFileWatcher;

	// physics clear
	delete myPhysWorld;

	// we can mark that the engine is done - wrap the threads
	myIsRunning = false;
	
	// get rid of pending objects
	ourGODeleteEnabled = true;
	while (!myAddQueue.empty())
	{
		myAddQueue.pop();
	}
	while (!myRemoveQueue.empty())
	{
		myRemoveQueue.pop();
	}
	myWorld.Clear();
	ourGODeleteEnabled = false;

	for (TerrainEntity terrain : myTerrains)
	{
		delete terrain.myTerrain;
		delete terrain.myVisualObject;
		if (terrain.myPhysComponent)
		{
			delete terrain.myPhysComponent;
		}
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

void Game::ForEachDebugDrawer(const DebugDrawerCallback& aFunc)
{
	aFunc(myDebugDrawer);
	aFunc(myPhysWorld->GetDebugDrawer());
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

Renderable& Game::CreateRenderable(GameObject& aGO)
{
	std::lock_guard lock(myRenderablesMutex);
	return myRenderables.Allocate(VisualObject{}, &aGO);
}

void Game::DeleteRenderable(Renderable& aRenderable)
{
	std::lock_guard lock(myRenderablesMutex);
	myRenderables.Free(aRenderable);
}

void Game::AddTerrain(Terrain* aTerrain, Handle<Pipeline> aPipeline)
{
#ifdef ASSERT_MUTEX
	AssertLock assertLock(myTerrainsMutex);
#endif

	myTerrains.push_back({ aTerrain, nullptr, nullptr });
	TerrainEntity& terrainEntity = myTerrains[myTerrains.size() - 1];
	Handle<Texture> terrainTexture = aTerrain->GetTextureHandle();
	terrainEntity.myVisualObject = new VisualObject();
	terrainEntity.myVisualObject->SetPipeline(aPipeline);
	terrainEntity.myVisualObject->SetTexture(terrainTexture);

	terrainTexture->ExecLambdaOnLoad([
		physWorld = myPhysWorld, 
		entity = &terrainEntity
	](const Resource* aRes) {
		Terrain* terrain = entity->myTerrain;
		VisualObject* visObject = entity->myVisualObject;
		Transform transf = visObject->GetTransform();
		glm::vec3 pos = terrain->GetPhysShape()->AdjustPositionForRecenter(transf.GetPos());
		transf.SetPos(pos);

		PhysicsComponent* physComp = new PhysicsComponent();
		physComp->CreateOwnerlessPhysicsEntity(0,
			terrain->GetPhysShape(),
			transf.GetMatrix()
		);
		physComp->GetPhysicsEntity().SetCollisionFlags(
			physComp->GetPhysicsEntity().GetCollisionFlags()
			| btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT
		);

		physComp->RequestAddToWorld(*physWorld);
		entity->myPhysComponent = physComp;
	});
}

void Game::AddGameObjects()
{
#ifdef ASSERT_MUTEX
	AssertLock assertLock(myGOMutex);
#endif
	tbb::spin_mutex::scoped_lock spinlock(myAddLock);
	while (myAddQueue.size())
	{
		Handle<GameObject> go = myAddQueue.front();
		myAddQueue.pop();
		myWorld.Add(go);
	}
}

void Game::BeginFrame()
{
	Profiler::ScopedMark profile(__func__);
	Input::Update();
	
	myImGUISystem->NewFrame(myDeltaTime);

	myDebugDrawer.BeginFrame();
}

void Game::Update()
{
	Profiler::ScopedMark profile(__func__);
	if (Input::GetKey(Input::Keys::Escape) || myShouldEnd)
	{
		mySettings.myIsPaused = myIsRunning = false;
		return;
	}

	if (Input::GetKeyPressed(Input::Keys::B))
	{
		mySettings.myIsPaused = !mySettings.myIsPaused;
	}

	if (Input::GetKeyPressed(Input::Keys::G))
	{
		myRenderThread->RequestSwitch();
	}
}

void Game::PhysicsUpdate()
{
	if (!mySettings.myIsPaused)
	{
		Profiler::ScopedMark profile(__func__);

		const bool isDrawing = myPhysWorld->IsDebugDrawingEnabled();
		if (mySettings.myDrawPhysicsDebug != isDrawing)
		{
			myPhysWorld->SetDebugDrawing(mySettings.myDrawPhysicsDebug);
		}

		myPhysWorld->Simulate(myDeltaTime);
	}
}

void Game::AnimationUpdate()
{
	if (!mySettings.myIsPaused)
	{
		Profiler::ScopedMark profile(__func__);
		myAnimationSystem->Update(myDeltaTime);
	}
}

void Game::Render()
{
	Profiler::ScopedMark profile("Game::Render");
	Graphics& graphics = *myRenderThread->GetGraphics();
	myCamera->Recalculate(graphics.GetWidth(), graphics.GetHeight());
	myRenderThread->Gather();
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

	if (Input::GetKeyPressed(Input::Keys::K))
	{
		mySettings.myUseWireframe = !mySettings.myUseWireframe;
	}

	myTopBar.Draw();

	Input::PostUpdate();
}

void Game::RemoveGameObjects()
{
	Profiler::ScopedMark profile(__func__);
	
#ifdef ASSERT_MUTEX
	AssertLock assertLock(myGOMutex);
#endif

	tbb::spin_mutex::scoped_lock spinlock(myRemoveLock);
	ourGODeleteEnabled = true;
	while (myRemoveQueue.size())
	{
		Handle<GameObject> go = myRemoveQueue.front();
		myWorld.Remove(go);
		myRemoveQueue.pop();
	}
	ourGODeleteEnabled = false;
}