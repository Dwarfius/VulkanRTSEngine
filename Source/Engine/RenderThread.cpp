#include "Precomp.h"
#include "RenderThread.h"

#include "Graphics/GL/GraphicsGL.h"
#include "Graphics/VK/GraphicsVK.h"
#include "Input.h"
#include "Game.h"
#include "VisualObject.h"
#include "Graphics/Adapters/UniformAdapter.h"
#include "Graphics/Adapters/TerrainAdapter.h"
#include "Terrain.h"
#include "Graphics/RenderPasses/GenericRenderPasses.h"

#include <Graphics/Camera.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/GPUPipeline.h>

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

void RenderThread::AddRenderable(VisualObject* aVO)
{
	// TODO: [[likely]]
	if (!aVO->IsResolved())
	{
		myResolveQueue.emplace(aVO);
	}
	else
	{
		std::vector<const VisualObject*>& buffer = myRenderables.GetWrite();
		buffer.push_back(aVO);
	}
}

void RenderThread::AddDebugRenderable(const DebugDrawer* aDebugDrawer)
{
	if (!aDebugDrawer->GetCurrentVertexCount())
	{
		return;
	}

	myDebugDrawers.GetWrite().push_back(aDebugDrawer);
}

void RenderThread::SubmitRenderables()
{
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
	Game::GetInstance()->GetCamera()->Recalculate();

	// the current render queue has been used up, we can fill it up again
	myGraphics->BeginGather();

	{
		std::queue<VisualObject*> delayQueue;
		// try to resolve all the pending VisualObjects
		while (!myResolveQueue.empty())
		{
			VisualObject* vo = myResolveQueue.front();
			myResolveQueue.pop();
			if (vo->Resolve())
			{
				myRenderables.GetWrite().push_back(vo);
			}
			else
			{
				delayQueue.emplace(vo);
			}
		}

		while (!delayQueue.empty())
		{
			myResolveQueue.emplace(delayQueue.front());
			delayQueue.pop();
		}
	}

	// processing our renderables
	myRenderables.Advance();
	const std::vector<const VisualObject*>& currRenderables = myRenderables.GetRead();
	myRenderables.GetWrite().clear();

	// TODO: this is most probably overkill considering that Render call is lightweight
	// need to look into batching those
	const Camera& cam = *Game::GetInstance()->GetCamera();
	/*tbb::parallel_for_each(myRenderables.begin(), myRenderables.end(),
		[&](const VisualObject* aVO)*/
	for (const VisualObject* aVO : currRenderables)
	{
		// building a render job
		RenderJob renderJob(
			aVO->GetPipeline(),
			aVO->GetModel(),
			{ aVO->GetTexture() }
		);
		IRenderPass::Category jobCategory = [aVO] {
			// TODO: get rid of this double category.
			// The VO has a category only to support rendering at the
			// moment, and there's no need for duplicating the enum!
			switch (aVO->GetCategory())
			{
			case VisualObject::Category::GameObject:
				return IRenderPass::Category::Renderables;
			case VisualObject::Category::Terrain:
				return IRenderPass::Category::Terrain;
			default:
				ASSERT(false);
			}
		}();

		if (!myGraphics->CanRender(jobCategory, renderJob))
		{
			continue;
		}

		if (renderJob.myModel.IsValid() 
			&& !cam.CheckSphere(aVO->GetTransform().GetPos(), aVO->GetRadius()))
		{
			continue;
		}

		// TODO: refactor away to Graphics!
		// updating the uniforms - grabbing game state!
		const size_t uboCount = aVO->GetPipeline().Get<const GPUPipeline>()->GetDescriptorCount();
		for (size_t i = 0; i < uboCount; i++)
		{
			UniformBlock& uniformBlock = aVO->GetUniformBlock(i);
			const UniformAdapter& adapter = aVO->GetUniformAdapter(i);
			adapter.FillUniformBlock(cam, uniformBlock);
		}
		renderJob.SetUniformSet(aVO->GetUniforms());
			
		switch (jobCategory)
		{
		case IRenderPass::Category::Renderables:
		{
			IRenderPass::IParams params;
			params.myDistance = 0; // TODO: implement it
			myGraphics->Render(jobCategory, renderJob, params);
		}
			break;
		case IRenderPass::Category::Terrain:
		{
			const Terrain* terrain = Game::GetInstance()->GetTerrain(glm::vec3());
			TerrainRenderParams params;
			params.myDistance = 0;
			const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(terrain);
			params.myTileCount = gridTiles.x * gridTiles.y;
			myGraphics->Render(jobCategory, renderJob, params);
		}
			break;
		}
	}//);

	// schedule drawing out our debug drawings
	myDebugDrawers.Advance();
	const std::vector<const DebugDrawer*>& debugDrawers = myDebugDrawers.GetRead();
	myDebugDrawers.GetWrite().clear();

	myGraphics->PrepareLineCache(0); // TODO: implement PrepareLineCache properly
	for (const DebugDrawer* drawer : debugDrawers)
	{
		myGraphics->RenderDebug(cam, *drawer);
	}

	myGraphics->Display();

	myHasWorkPending = false;
}