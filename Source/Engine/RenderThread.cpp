#include "Precomp.h"
#include "RenderThread.h"

#include "Graphics/GL/GraphicsGL.h"
#include "Graphics/VK/GraphicsVK.h"
#include "Input.h"
#include "Game.h"
#include "VisualObject.h"
#include "Graphics/Adapters/UniformAdapter.h"
#include "Terrain.h"
#include "Graphics/RenderPasses/GenericRenderPasses.h"

#include <Graphics/Camera.h>
#include <Graphics/Pipeline.h>


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
		myGraphics = make_unique<GraphicsVK>();
		Game::GetInstance()->GetCamera()->InvertProj();
	}
	else
#endif // USE_VULKAN
	{
		myGraphics = make_unique<GraphicsGL>(anAssetTracker);
		anAssetTracker.SetGPUAllocator(myGraphics.get());
	}
	myGraphics->SetMaxThreads(thread::hardware_concurrency());
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

void RenderThread::AddRenderable(const VisualObject* aVO)
{
	vector<const VisualObject*>& buffer = myTrippleRenderables.GetWrite();
	buffer.push_back(aVO);
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
		this_thread::yield();
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

	// processing our renderables
	myTrippleRenderables.Advance();
	const vector<const VisualObject*>& myRenderables = myTrippleRenderables.GetRead();
	myTrippleRenderables.GetWrite().clear();

	// TODO: this is most probably overkill considering that Render call is lightweight
	// need to look into batching those
	const Camera& cam = *Game::GetInstance()->GetCamera();
	/*tbb::parallel_for_each(myRenderables.begin(), myRenderables.end(),
		[&](const VisualObject* aVO)*/
	for(const VisualObject* aVO : myRenderables)
	{
		if (cam.CheckSphere(aVO->GetTransform().GetPos(), aVO->GetRadius()))
		{
			// updating the uniforms - grabbing game state!
			size_t uboCount = aVO->GetPipeline()->GetDescriptorCount();
			for (size_t i = 0; i < uboCount; i++)
			{
				UniformBlock& uniformBlock = aVO->GetUniformBlock(i);
				const UniformAdapter& adapter = aVO->GetUniformAdapter(i);
				adapter.FillUniformBlock(cam, uniformBlock);
			}

			// building a render job
			RenderJob renderJob(
				aVO->GetPipeline(), 
				aVO->GetModel(), 
				{ aVO->GetTexture() }, 
				aVO->GetUniforms() 
			);
			switch (aVO->GetCategory())
			{
			case VisualObject::Category::GameObject:
			{
				IRenderPass::IParams params;
				params.myDistance = 0; // TODO: implement it
				myGraphics->Render(IRenderPass::Category::Renderables, cam, renderJob, params);
			}
				break;
			case VisualObject::Category::Terrain:
			{
				const Terrain* terrain = Game::GetInstance()->GetTerrain(glm::vec3());
				glm::vec3 scale = aVO->GetTransform().GetScale();
				TerrainRenderParams params;
				params.myDistance = 0;
				params.mySize = scale * glm::vec3(terrain->GetWidth(), 0, terrain->GetDepth());
				myGraphics->Render(IRenderPass::Category::Terrain, cam, renderJob, params);
			}
				break;
			}
		}
	}//);

	// schedule drawing out our debug drawings
	myDebugDrawers.Advance();
	const vector<const DebugDrawer*>& debugDrawers = myDebugDrawers.GetRead();
	myDebugDrawers.GetWrite().clear();

	myGraphics->PrepareLineCache(0); // TODO: implement PrepareLineCache properly
	for (const DebugDrawer* drawer : debugDrawers)
	{
		myGraphics->RenderDebug(cam, *drawer);
	}

	myGraphics->Display();

	myHasWorkPending = false;
}