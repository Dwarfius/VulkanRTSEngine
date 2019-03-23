#include "Precomp.h"
#include "RenderThread.h"

#include "Graphics/GL/GraphicsGL.h"
#include "Graphics/VK/GraphicsVK.h"
#include "Input.h"
#include "Game.h"
#include "Camera.h"
#include "VisualObject.h"

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

void RenderThread::AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor)
{
	vector<PosColorVertex>& buffer = myTrippleLines.GetWrite();
	buffer.push_back(PosColorVertex{ aFrom, aColor });
	buffer.push_back(PosColorVertex{ aTo, aColor });
}

void RenderThread::AddLines(const vector<PosColorVertex>& aLineCache)
{
	vector<PosColorVertex>& buffer = myTrippleLines.GetWrite();
	buffer.insert(buffer.end(), aLineCache.begin(), aLineCache.end());
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

	myGraphics->ResetRenderCalls();

	// update the mvp
	Game::GetInstance()->GetCamera()->Recalculate();

	// the current render queue has been used up, we can fill it up again
	myGraphics->BeginGather();

	// processing our renderables
	myTrippleRenderables.Swap();
	const vector<const VisualObject*>& myRenderables = myTrippleRenderables.GetRead();
	myTrippleRenderables.GetWrite().clear();

	// TODO: this is most probably overkill considering that Render call is lightweight
	// need to look into batching those
	const Camera& cam = *Game::GetInstance()->GetCamera();
	tbb::parallel_for_each(myRenderables.begin(), myRenderables.end(),
		[&](const VisualObject* aVO)
	{
		if (cam.CheckSphere(aVO->GetTransform().GetPos(), aVO->GetRadius()))
		{
			myGraphics->Render(cam, aVO);
		}
	});

	// schedule drawing out our debug drawings
	myTrippleLines.Swap();
	const vector<PosColorVertex>& myLines = myTrippleLines.GetRead();
	myTrippleLines.GetWrite().clear();

	myGraphics->PrepareLineCache(myLines.size());
	myGraphics->DrawLines(cam, myLines);

	myGraphics->Display();

	myHasWorkPending = false;
}