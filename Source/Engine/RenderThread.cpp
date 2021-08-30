#include "Precomp.h"
#include "RenderThread.h"

#include "Graphics/GL/GraphicsGL.h"
#include "Graphics/VK/GraphicsVK.h"
#include "Input.h"
#include "Game.h"
#include "VisualObject.h"
#include "Graphics/Adapters/TerrainAdapter.h"
#include "Graphics/Adapters/AdapterSourceData.h"
#include "Terrain.h"
#include "Graphics/RenderPasses/GenericRenderPasses.h"
#include "Graphics/RenderPasses/DebugRenderPass.h"
#include "Graphics/NamedFrameBuffers.h"

#include <Graphics/Camera.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/UniformAdapter.h>
#include <Graphics/FrameBuffer.h>

#include <Core/Profiler.h>

RenderThread::RenderThread()
	: myIsUsingVulkan(false)
	, myHasWorkPending(false)
	, myNeedsSwitch(false)
{
}

RenderThread::~RenderThread()
{
	myGraphics->CleanUp();
}

void RenderThread::Init(bool anUseVulkan, AssetTracker& anAssetTracker)
{
	Profiler::ScopedMark profile("RenderThread::Init");
	myIsUsingVulkan = anUseVulkan;

#ifdef USE_VULKAN
	if (myIsUsingVulkan)
	{
		myGraphics = std::make_unique<GraphicsVK>();
		Game::GetInstance()->GetCamera()->InvertProj();
	}
	else
#endif // USE_VULKAN
	{
		myGraphics = std::make_unique<GraphicsGL>(anAssetTracker);
	}
	myGraphics->SetMaxThreads(std::thread::hardware_concurrency());
	myGraphics->Init();
	myGraphics->AddRenderPass(new DefaultRenderPass());
	myGraphics->AddRenderPass(new TerrainRenderPass());
	myGraphics->AddRenderPass(new DebugRenderPass(
		*myGraphics, anAssetTracker.GetOrCreate<Pipeline>("Engine/debug.ppl")
	));

	myGraphics->AddNamedFrameBuffer(DefaultFrameBuffer::kName, DefaultFrameBuffer::kDescriptor);

	Input::SetWindow(myGraphics->GetWindow());
}

void RenderThread::Work()
{
	myHasWorkPending = true;
}

GLFWwindow* RenderThread::GetWindow() const
{
	return myGraphics->GetWindow();
}

void RenderThread::AddRenderable(const VisualObject& aVO, const GameObject& aGO)
{
	ASSERT_STR(aVO.IsResolved(), "Visual Object hasn't been resolved - nothing to render!");
	std::vector<GameObjectRenderable>& buffer = myRenderables.GetWrite();
	buffer.push_back({ aVO, aGO });
}

void RenderThread::AddTerrainRenderable(const VisualObject& aVO, const Terrain& aTerrain)
{
	ASSERT_STR(aVO.IsResolved(), "Visual Object hasn't been resolved - nothing to render!");
	std::vector<TerrainRenderable>& buffer = myTerrainRenderables.GetWrite();
	buffer.push_back({ aVO, aTerrain });
}

void RenderThread::AddDebugRenderable(const DebugDrawer& aDebugDrawer)
{
	if (!aDebugDrawer.GetCurrentVertexCount())
	{
		return;
	}

	myDebugDrawers.GetWrite().push_back({ aDebugDrawer });
}

void RenderThread::SubmitRenderables()
{
	Profiler::ScopedMark profile("RenderThread::SubmitRenderables");
	if (Game::GetInstance()->IsPaused() || !myHasWorkPending)
	{
		std::this_thread::yield();
	}

	if (!myIsUsingVulkan)
	{
		ASSERT_STR(glfwGetCurrentContext(), "Missing current GL context!");
	}
	
#ifdef USE_VULKAN
	if (myNeedsSwitch)
	{
		// TODO: replace with Init call
		printf("[Info] Switching renderer...\n");
		myIsUsingVulkan = !myIsUsingVulkan;

		myGraphics->CleanUp();

		printf("\n");
		if (myIsUsingVulkan)
		{
			myGraphics = make_unique<GraphicsVK>();
		}
		else
		{
			myGraphics = make_unique<GraphicsGL>();
		}
		myGraphics->SetMaxThreads(thread::hardware_concurrency());
		myGraphics->Init(*myTerrains);

		Game::GetInstance()->GetCamera()->InvertProj();
		Input::SetWindow(myGraphics->GetWindow());

		myNeedsSwitch = false;
	}
#endif // USE_VULKAN

	// update the mvp
	Game::GetInstance()->GetCamera()->Recalculate(myGraphics->GetWidth(), myGraphics->GetHeight());

	// the current render queue has been used up, we can fill it up again
	myGraphics->BeginGather();

	// TODO: Camera needs to be passed in rather than grabbed like this
	const Camera& cam = *Game::GetInstance()->GetCamera();
	ScheduleGORenderables(cam);
	ScheduleTerrainRenderables(cam);
	ScheduleDebugRenderables(cam);

	myGraphics->Display();

	myHasWorkPending = false;
}

void RenderThread::ScheduleGORenderables(const Camera& aCam)
{
	myRenderables.Advance();
	const std::vector<GameObjectRenderable>& currRenderables = myRenderables.GetRead();
	myRenderables.GetWrite().clear();

	Profiler::ScopedMark visualObjectProfile("RenderThread::ScheduleGORender");
	DefaultRenderPass* renderPass = myGraphics->GetRenderPass<DefaultRenderPass>();
	for (const GameObjectRenderable& renderable : currRenderables)
	{
		// building a render job
		const VisualObject& visualObj = renderable.myVisualObject;
		RenderJob renderJob(
			visualObj.GetPipeline(),
			visualObj.GetModel(),
			{ visualObj.GetTexture() }
		);
		
		if (!renderPass->HasResources(renderJob))
		{
			continue;
		}

		if (!aCam.CheckSphere(visualObj.GetTransform().GetPos(), visualObj.GetRadius()))
		{
			continue;
		}

		// updating the uniforms - grabbing game state!
		UniformAdapterSource source{
			aCam,
			&renderable.myGameObject,
			visualObj
		};
		const GPUPipeline* gpuPipeline = visualObj.GetPipeline().Get<const GPUPipeline>();
		const size_t uboCount = gpuPipeline->GetDescriptorCount();
		for (size_t i = 0; i < uboCount; i++)
		{
			UniformBlock& uniformBlock = visualObj.GetUniformBlock(i);
			const UniformAdapter& adapter = gpuPipeline->GetAdapter(i);
			adapter.FillUniformBlock(source, uniformBlock);
		}
		renderJob.SetUniformSet(visualObj.GetUniforms());

		IRenderPass::IParams params;
		params.myDistance = glm::distance(
			aCam.GetTransform().GetPos(), 
			renderable.myGameObject.GetWorldTransform().GetPos()
		);
		renderPass->AddRenderable(renderJob, params);
	}
}

void RenderThread::ScheduleTerrainRenderables(const Camera& aCam)
{
	myTerrainRenderables.Advance();
	const std::vector<TerrainRenderable>& currRenderables = myTerrainRenderables.GetRead();
	myTerrainRenderables.GetWrite().clear();

	Profiler::ScopedMark visualObjectProfile("RenderThread::ScheduleTerrainRender");
	TerrainRenderPass* renderPass = myGraphics->GetRenderPass<TerrainRenderPass>();
	for (const TerrainRenderable& renderable : currRenderables)
	{
		// building a render job
		const VisualObject& visualObj = renderable.myVisualObject;
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
			&& !aCam.CheckSphere(visualObj.GetTransform().GetPos(), visualObj.GetRadius()))
		{
			continue;
		}

		// updating the uniforms - grabbing game state!
		TerrainUniformAdapterSource source{
			aCam,
			nullptr,
			visualObj,
			renderable.myTerrain,
		};
		const GPUPipeline* gpuPipeline = visualObj.GetPipeline().Get<const GPUPipeline>();
		const size_t uboCount = gpuPipeline->GetDescriptorCount();
		for (size_t i = 0; i < uboCount; i++)
		{
			UniformBlock& uniformBlock = visualObj.GetUniformBlock(i);
			const UniformAdapter& adapter = gpuPipeline->GetAdapter(i);
			adapter.FillUniformBlock(source, uniformBlock);
		}
		renderJob.SetUniformSet(visualObj.GetUniforms());

		TerrainRenderParams params;
		params.myDistance = glm::distance(
			aCam.GetTransform().GetPos(),
			visualObj.GetTransform().GetPos()
		);
		const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(renderable.myTerrain);
		params.myTileCount = gridTiles.x * gridTiles.y;
		renderPass->AddRenderable(renderJob, params);
	}
}

void RenderThread::ScheduleDebugRenderables(const Camera& aCam)
{
	myDebugDrawers.Advance();
	const std::vector<DebugRenderable>& debugDrawers = myDebugDrawers.GetRead();
	myDebugDrawers.GetWrite().clear();

	Profiler::ScopedMark debugProfile("RenderThread::ScheduleDebugRender");
	DebugRenderPass* renderPass = myGraphics->GetRenderPass<DebugRenderPass>();
	if (!renderPass->IsReady())
	{
		return;
	}

	renderPass->SetCamera(0, aCam, *myGraphics);
	for (const DebugRenderable& renderable : debugDrawers)
	{
		renderPass->AddDebugDrawer(0, renderable.myDebugDrawer);
	}
}