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
	, initialised(false)
{
}

RenderThread::~RenderThread()
{
	if(renderThread.get())
		renderThread->join();
}

void RenderThread::Init(bool useVulkan, const vector<Terrain>* aTerrainSet)
{
	usingVulkan = useVulkan;
	terrains = aTerrainSet;
	renderThread = make_unique<thread>(&RenderThread::InternalLoop, this);
}

void RenderThread::Work()
{
	if (Input::GetKeyPressed('G'))
	{
		needsSwitch = true;
	}

	workPending = true;
}

void RenderThread::InternalLoop()
{
	if (usingVulkan)
	{
		graphics = make_unique<GraphicsVK>();
		Game::GetInstance()->GetCamera()->InvertProj();
	}
	else
		graphics = make_unique<GraphicsGL>();
	graphics->Init(*terrains);
	Input::SetWindow(graphics->GetWindow());
	initialised = true;

	while (Game::GetInstance()->IsRunning())
	{
		if (Game::GetInstance()->IsPaused() || !workPending)
			this_thread::yield();

		if (needsSwitch)
		{
			initialised = false;
			printf("[Info] Switching renderer...\n");
			usingVulkan = !usingVulkan;

			graphics->CleanUp();

			printf("\n");
			if (usingVulkan)
				graphics = make_unique<GraphicsVK>();
			else
				graphics = make_unique<GraphicsGL>();
			graphics->Init(*terrains);

			Game::GetInstance()->GetCamera()->InvertProj();
			Input::SetWindow(graphics->GetWindow());

			needsSwitch = false;
			initialised = true;
		}

		graphics->ResetRenderCalls();

		// update the mvp
		Game::GetInstance()->GetCamera()->Recalculate();

		// the current render queue has been used up, we can fill it up again
		graphics->BeginGather();

		const Camera& cam = *Game::GetInstance()->GetCamera();
		const tbb::concurrent_vector<GameObject*>& gameObjects = Game::GetInstance()->GetGameObjects();
		// TODO: fix this parrallel for
		/*tbb::parallel_for_each(gameObjects.begin(), gameObjects.end(), [&](GameObject* go) {
			if (go->GetIndex() != numeric_limits<size_t>::max() &&
				cam.CheckSphere(go->GetTransform()->GetPos(), go->GetRadius()))
				graphics->Render(cam, go, 0);
		});*/
		for (size_t i = 0, end = gameObjects.size(); i < end; i++)
		{
			GameObject* go = gameObjects[i];
			if (go->GetIndex() != numeric_limits<size_t>::max() &&
				cam.CheckSphere(go->GetTransform()->GetPos(), go->GetRadius()))
				graphics->Render(cam, go, 0);
		}

		graphics->Display();

		workPending = false;
	}

	graphics->CleanUp();
}