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
	: usingVulkan(false)
	, terrains(nullptr)
	, workPending(false)
	, needsSwitch(false)
{
}

RenderThread::~RenderThread()
{
	graphics->CleanUp();
}

void RenderThread::Init(bool useVulkan, const vector<Terrain>* aTerrainSet)
{
	usingVulkan = useVulkan;
	terrains = aTerrainSet;

	if (usingVulkan)
	{
		graphics = make_unique<GraphicsVK>();
		Game::GetInstance()->GetCamera()->InvertProj();
	}
	else
	{
		graphics = make_unique<GraphicsGL>();
	}
	graphics->Init(*terrains);
	Input::SetWindow(graphics->GetWindow());
}

void RenderThread::Work()
{
	workPending = true;
}

GLFWwindow* RenderThread::GetWindow() const
{
	return graphics->GetWindow();
}

void RenderThread::AddRenderable(const GameObject* go)
{
	myTrippleRenderables.Add(go);
}

void RenderThread::InternalLoop()
{
	if (Game::GetInstance()->IsPaused() || !workPending)
		this_thread::yield();

	// TODO: introduce proper assert macros
	if (!usingVulkan)
	{
		assert(glfwGetCurrentContext());
	}
	
	if (needsSwitch)
	{
		printf("[Info] Switching renderer...\n");
		usingVulkan = !usingVulkan;

		graphics->CleanUp();

		printf("\n");
		if (usingVulkan)
			graphics = make_unique<GraphicsVK>();
		else
		{
			graphics = make_unique<GraphicsGL>();
			glfwMakeContextCurrent(graphics->GetWindow());
		}
		graphics->Init(*terrains);

		Game::GetInstance()->GetCamera()->InvertProj();
		Input::SetWindow(graphics->GetWindow());

		needsSwitch = false;
	}

	graphics->ResetRenderCalls();

	// update the mvp
	Game::GetInstance()->GetCamera()->Recalculate();

	// the current render queue has been used up, we can fill it up again
	graphics->BeginGather();

	const vector<const GameObject*>& myRenderables = myTrippleRenderables.GetBuffer();
	myTrippleRenderables.Advance();

	const Camera& cam = *Game::GetInstance()->GetCamera();
	tbb::parallel_for_each(myRenderables.begin(), myRenderables.end(),
		[&](const GameObject* go) {
		if (cam.CheckSphere(go->GetTransform()->GetPos(), go->GetRadius()))
			graphics->Render(cam, go, 0);
	});

	graphics->Display();

	workPending = false;
}