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
#include "Graphics/Adapters/TerrainAdapter.h"
#include "Graphics/Adapters/SkeletonAdapter.h"
#include "Graphics/RenderPasses/FinalCompositeRenderPass.h"
#include "Graphics/RenderPasses/DebugRenderPass.h"
#include "Graphics/RenderPasses/GenericRenderPasses.h"
#include "Systems/ImGUI/ImGUISystem.h"
#include "Systems/ImGUI/ImGUIRendering.h"
#include "Animation/AnimationSystem.h"
#include "RenderThread.h"
#include "UIWidgets/EntitiesWidget.h"
#include "UIWidgets/ObjImportDialog.h"
#include "UIWidgets/GltfImportDialog.h"
#include "UIWidgets/TextureImportDialog.h"
#include "UIWidgets/ShaderCreateDialog.h"
#include "UIWidgets/PipelineCreateDialog.h"
#include "Systems/ProfilerUI.h"

#include <Core/StaticString.h>
#include <Core/Profiler.h>
#include <Core/Resources/AssetTracker.h>

#include <Graphics/Camera.h>
#include <Graphics/Graphics.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/GPUPipeline.h>
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
constexpr StaticString kHeightmapName("Tynemouth-tangrams.img");

namespace UIWidgets
{
	void DrawCameraInfo(bool& aIsVisible, const Game& aGame)
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
	, myIsPaused(false)
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

	myRenderThread = new RenderThread();

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

void Game::Init(bool aUseDefaultCompositePass)
{
	Profiler::ScopedMark profile("Game::Init");

	constexpr static uint32_t kInitReserve = 4000;
	myGameObjects.reserve(kInitReserve);

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

	myRenderThread->AddRenderContributor([this](Graphics& aGraphics) {
		ScheduleRenderables(aGraphics);
	});
	myImGUISystem->Init(*GetWindow());

	// setting up a task tree
	{
		GameTask task(Tasks::BeginFrame, [this] { BeginFrame(); });
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::AddGameObjects, [this] { AddGameObjects(); });
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::PhysicsUpdate, [this] { PhysicsUpdate(); });
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::AnimationUpdate, [this] { AnimationUpdate(); });
		task.AddDependency(Tasks::PhysicsUpdate);
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::GameUpdate, [this] { Update(); });
		task.AddDependency(Tasks::BeginFrame);
		task.AddDependency(Tasks::AddGameObjects);
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::RemoveGameObjects, [this] { RemoveGameObjects(); });
		task.AddDependency(Tasks::GameUpdate);
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::UpdateEnd, [this] { UpdateEnd(); });
		task.AddDependency(Tasks::RemoveGameObjects);
		myTaskManager->AddTask(task);

		task = GameTask(Tasks::Render, [this] { Render(); });
		task.AddDependency(Tasks::UpdateEnd);
		myTaskManager->AddTask(task);

		// TODO: will need to fix up audio
		task = GameTask(Tasks::UpdateAudio, [this] { UpdateAudio(); });
		task.AddDependency(Tasks::UpdateEnd);
		myTaskManager->AddTask(task);
	}

	myTopBar.Register("Widgets/Demo", [](bool& aIsVisible) {
		ImGui::ShowDemoWindow(&aIsVisible);
	});
	myTopBar.Register("Widgets/Camera Info", [&](bool& aIsVisible) {
		UIWidgets::DrawCameraInfo(aIsVisible, *this);
	});
	myTopBar.Register("Widgets/Profiler", &UIWidgets::DrawProfilerUI);
	myTopBar.Register("Widgets/Entities View",
		[&, entitiesWidget = EntitiesWidget()](bool& aIsVisible) mutable {
		entitiesWidget.DrawDialog(*this, aIsVisible);
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

	// we can mark that the engine is done - wrap the threads
	myIsRunning = false;
	// get rid of pending objects
	ourGODeleteEnabled = true;
	while (myAddQueue.size())
	{
		myAddQueue.pop();
	}
	ourGODeleteEnabled = false;

	// get rid of tracked objects
	for (auto [key, goHandle] : myGameObjects)
	{
		RemoveGameObject(goHandle);
	}
	RemoveGameObjects();
	ASSERT_STR(myGameObjects.empty(), "All objects should've been cleaned up!");

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

	// physics clear
	delete myPhysWorld;

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

void Game::AddRenderContributor(OnRenderCallback aCallback)
{
	myRenderThread->AddRenderContributor(aCallback);
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
		myGameObjects[go->GetUID()] = go;
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
	Profiler::ScopedMark profile("Game::Render");
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
		myGameObjects.erase(go->GetUID());
		myRemoveQueue.pop();
	}
	ourGODeleteEnabled = false;
}

void Game::ScheduleRenderables(Graphics& aGraphics)
{
	Profiler::ScopedMark debugProfile("Game::ScheduleRenderables");
	myCamera->Recalculate(aGraphics.GetWidth(), aGraphics.GetHeight());

	RenderGameObjects(aGraphics);
	RenderTerrains(aGraphics);
	RenderDebugDrawers(aGraphics);
}

void Game::RenderGameObjects(Graphics& aGraphics)
{
	Profiler::ScopedMark debugProfile("Game::RenderGameObjects");

#ifdef ASSERT_MUTEX
	AssertLock assertLock(myGOMutex);
#endif
	DefaultRenderPass* renderPass = aGraphics.GetRenderPass<DefaultRenderPass>();
	// TODO: get rid of single map, and use a separate vector for Renderables
	for (auto& [uid, gameObjHandle] : myGameObjects)
	{
		VisualObject* visObj = gameObjHandle->GetVisualObject();
		if (!visObj)
		{
			continue;
		}

		if (visObj->IsResolved() || visObj->Resolve())
		{
			// building a render job
			const GameObject* gameObject = gameObjHandle.Get();
			const VisualObject& visualObj = *visObj;
			RenderJob renderJob(
				visualObj.GetPipeline(),
				visualObj.GetModel(),
				{ visualObj.GetTexture() }
			);

			if (!renderPass->HasResources(renderJob))
			{
				continue;
			}

			if (!myCamera->CheckSphere(visualObj.GetCenter(), visualObj.GetRadius()))
			{
				continue;
			}

			// updating the uniforms - grabbing game state!
			UniformAdapterSource source{
				aGraphics,
				*myCamera,
				gameObject,
				visualObj
			};
			const GPUPipeline* gpuPipeline = visualObj.GetPipeline().Get<const GPUPipeline>();
			const size_t uboCount = gpuPipeline->GetAdapterCount();
			for (size_t i = 0; i < uboCount; i++)
			{
				UniformBlock& uniformBlock = visualObj.GetUniformBlock(i);
				const UniformAdapter& uniformAdapter = gpuPipeline->GetAdapter(i);
				uniformAdapter.Fill(source, uniformBlock);
			}
			renderJob.SetUniformSet(visualObj.GetUniforms());

			IRenderPass::IParams params;
			params.myDistance = glm::distance(
				myCamera->GetTransform().GetPos(),
				gameObject->GetWorldTransform().GetPos()
			);
			renderPass->AddRenderable(renderJob, params);
		}
	}
}

void Game::RenderTerrains(Graphics& aGraphics)
{
	Profiler::ScopedMark debugProfile("Game::RenderTerrains");

#ifdef ASSERT_MUTEX
	AssertLock assertLock(myTerrainsMutex);
#endif
	TerrainRenderPass* renderPass = aGraphics.GetRenderPass<TerrainRenderPass>();
	for (TerrainEntity& entity : myTerrains)
	{
		VisualObject* visObj = entity.myVisualObject;
		if (!visObj)
		{
			continue;
		}

		if (visObj->IsResolved() || visObj->Resolve())
		{
			// building a render job
			const Terrain& terrain = *entity.myTerrain;
			const VisualObject& visualObj = *visObj;
			RenderJob renderJob(
				visualObj.GetPipeline(),
				visualObj.GetModel(),
				{ visualObj.GetTexture() }
			);

			if (!renderPass->HasResources(renderJob))
			{
				continue;
			}

			if (visualObj.GetModel().IsValid()
				&& !myCamera->CheckSphere(visualObj.GetTransform().GetPos(), visualObj.GetRadius()))
			{
				continue;
			}

			// updating the uniforms - grabbing game state!
			TerrainAdapter::Source source{
				aGraphics,
				*myCamera,
				nullptr,
				visualObj,
				terrain
			};
			const GPUPipeline* gpuPipeline = visualObj.GetPipeline().Get<const GPUPipeline>();
			const size_t uboCount = gpuPipeline->GetAdapterCount();
			for (size_t i = 0; i < uboCount; i++)
			{
				UniformBlock& uniformBlock = visualObj.GetUniformBlock(i);
				const UniformAdapter& uniformAdapter = gpuPipeline->GetAdapter(i);
				uniformAdapter.Fill(source, uniformBlock);
			}
			renderJob.SetUniformSet(visualObj.GetUniforms());

			TerrainRenderParams params;
			params.myDistance = glm::distance(
				myCamera->GetTransform().GetPos(),
				visualObj.GetTransform().GetPos()
			);
			const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(terrain);
			params.myTileCount = gridTiles.x * gridTiles.y;
			renderPass->AddRenderable(renderJob, params);
		}
	}
}

void Game::RenderDebugDrawers(Graphics& aGraphics)
{
	Profiler::ScopedMark debugProfile("Game::RenderDebugDrawers");

	DebugRenderPass* renderPass = aGraphics.GetRenderPass<DebugRenderPass>();
	if (!renderPass->IsReady())
	{
		return;
	}

	renderPass->SetCamera(0, *myCamera, aGraphics);
	if (myDebugDrawer.GetCurrentVertexCount())
	{
		renderPass->AddDebugDrawer(0, myDebugDrawer);
	}

	if (myPhysWorld->GetDebugDrawer().GetCurrentVertexCount())
	{
		renderPass->AddDebugDrawer(0, myPhysWorld->GetDebugDrawer());
	}
}