#include "Common.h"
#include "RenderThread.h"
#include "Graphics.h"
#include "GraphicsGL.h"
#include "GraphicsVK.h"
#include "Input.h"
#include "Game.h"
#include "Camera.h"
#include "GameObject.h"

RenderThread::RenderThread()
	: myIsUsingVulkan(false)
	, myTerrains(nullptr)
	, myHasWorkPending(false)
	, myNeedsSwitch(false)
{
}

RenderThread::~RenderThread()
{
	myGraphics->CleanUp();
}

void RenderThread::Init(bool anUseVulkan, const vector<Terrain*>* aTerrainSet)
{
	assert(aTerrainSet);
	assert(aTerrainSet->size() > 0);

	myIsUsingVulkan = anUseVulkan;
	myTerrains = aTerrainSet;

	if (myIsUsingVulkan)
	{
		myGraphics = make_unique<GraphicsVK>();
		Game::GetInstance()->GetCamera()->InvertProj();
	}
	else
	{
		myGraphics = make_unique<GraphicsGL>();
	}
	myGraphics->SetMaxThreads(thread::hardware_concurrency());
	myGraphics->Init(*myTerrains);
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

void RenderThread::AddRenderable(const GameObject* aGo)
{
	vector<const GameObject*>& buffer = myTrippleRenderables.GetCurrent();
	buffer.push_back(aGo);
}

void RenderThread::InternalLoop()
{
	if (Game::GetInstance()->IsPaused() || !myHasWorkPending)
		this_thread::yield();

	if (!myIsUsingVulkan)
	{
		// TODO: introduce proper assert macros
		assert(glfwGetCurrentContext());
	}
	
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
			glfwMakeContextCurrent(myGraphics->GetWindow());
		}
		myGraphics->SetMaxThreads(thread::hardware_concurrency());
		myGraphics->Init(*myTerrains);

		Game::GetInstance()->GetCamera()->InvertProj();
		Input::SetWindow(myGraphics->GetWindow());

		myNeedsSwitch = false;
	}

	myGraphics->ResetRenderCalls();

	// update the mvp
	Game::GetInstance()->GetCamera()->Recalculate();

	// the current render queue has been used up, we can fill it up again
	myGraphics->BeginGather();

	const vector<const GameObject*>& myRenderables = myTrippleRenderables.GetCurrent();
	myTrippleRenderables.Advance();
	myTrippleRenderables.GetCurrent().clear();

	const Camera& cam = *Game::GetInstance()->GetCamera();
	tbb::parallel_for_each(myRenderables.begin(), myRenderables.end(),
		[&](const GameObject* aGo) {
		if (cam.CheckSphere(aGo->GetTransform().GetPos(), aGo->GetRadius()))
		{
			myGraphics->Render(cam, aGo);
		}
	});

	myGraphics->Display();

	myHasWorkPending = false;
}