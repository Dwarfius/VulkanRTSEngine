#include "Precomp.h"
#include "RenderThread.h"

#include "Graphics/GL/GraphicsGL.h"
#include "Graphics/VK/GraphicsVK.h"
#include "Input.h"
#include "Game.h"
#include "Graphics/RenderPasses/GenericRenderPasses.h"
#include "Graphics/RenderPasses/DebugRenderPass.h"
#include "Graphics/RenderPasses/FinalCompositeRenderPass.h"
#include "Graphics/NamedFrameBuffers.h"

#include <Graphics/Resources/Pipeline.h>

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

void RenderThread::Gather()
{
	myGraphics->BeginGather();
	{
#ifdef ASSERT_MUTEX
		AssertLock lock(myRenderCallbackMutex);
#endif

		for (OnRenderCallback callback : myRenderCallbacks)
		{
			callback(*myGraphics);
		}
	}
	myGraphics->EndGather();
	
	myHasWorkPending = true;
}

GLFWwindow* RenderThread::GetWindow() const
{
	return myGraphics->GetWindow();
}

void RenderThread::AddRenderContributor(OnRenderCallback aCallback)
{
#ifdef ASSERT_MUTEX
	AssertLock lock(myRenderCallbackMutex);
#endif
	myRenderCallbacks.push_back(aCallback);
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

	myGraphics->Display();

	myHasWorkPending = false;
}