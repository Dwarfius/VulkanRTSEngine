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
	if (Input::GetKeyPressed('G'))
	{
		needsSwitch = true;
	}

	workPending = true;
}

void RenderThread::AddRenderable(const GameObject* go)
{
	myRenderables[go->GetUID()] = go;
}

void RenderThread::RemoveRenderable(const UID& uid)
{
	myRenderables.erase(uid);
}

void RenderThread::InternalLoop()
{
	if (Game::GetInstance()->IsPaused() || !workPending)
		this_thread::yield();

	assert(glfwGetCurrentContext());
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

	const Camera& cam = *Game::GetInstance()->GetCamera();
	// TODO: fix this parrallel for
	tbb::parallel_for_each(myRenderables.begin(), myRenderables.end(),
		[&](const pair<UID, const GameObject*> iterPair) {
		const GameObject* go = iterPair.second;
		if (cam.CheckSphere(go->GetTransform()->GetPos(), go->GetRadius()))
			graphics->Render(cam, go, 0);
	});

	graphics->Display();

	workPending = false;
}