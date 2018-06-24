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

void RenderThread::AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor)
{
	vector<PhysicsDebugDrawer::LineDraw>& buffer = myTrippleLines.GetCurrent();
	buffer.push_back(PhysicsDebugDrawer::LineDraw{ aFrom, aColor, aTo, aColor });
}

void RenderThread::AddLines(const vector<PhysicsDebugDrawer::LineDraw>& aLineCache)
{
	vector<PhysicsDebugDrawer::LineDraw>& buffer = myTrippleLines.GetCurrent();
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

	// processing our renderables
	const vector<const GameObject*>& myRenderables = myTrippleRenderables.GetCurrent();
	myTrippleRenderables.Advance();
	myTrippleRenderables.GetCurrent().clear();

	// TODO: this is most probably overkill considering that Render call is lightweight
	// need to look into batching those
	const Camera& cam = *Game::GetInstance()->GetCamera();
	tbb::parallel_for_each(myRenderables.begin(), myRenderables.end(),
		[&](const GameObject* aGo) {
		if (cam.CheckSphere(aGo->GetTransform().GetPos(), aGo->GetRadius()))
		{
			myGraphics->Render(cam, aGo);
		}
	});

	// schedule drawing out our debug drawings
	const vector<PhysicsDebugDrawer::LineDraw>& myLines = myTrippleLines.GetCurrent();
	myTrippleLines.Advance();
	myTrippleLines.GetCurrent().clear();

	myGraphics->PrepareLineCache(myLines.size());
	myGraphics->DrawLines(cam, myLines);

	myGraphics->Display();

	myHasWorkPending = false;
}