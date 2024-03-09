#include "Precomp.h"

#include "StressTest.h"

#include <Engine/Game.h>

void glfwErrorReporter(int code, const char* desc)
{
	std::println("GLFW error({}): {}", code, desc);
}

int main()
{
	// TODO: get rid of this and all rand() calls
	srand(static_cast<uint32_t>(time(0)));

	// initialize the game engine
	Game* game = new Game(&glfwErrorReporter);
	game->Init(true);

	// setup and inject our test mode task
	StressTest* testScenario = new StressTest(*game);
	constexpr GameTask::Type kTestUpdateTask = Game::Tasks::Last + 1;
	GameTask testUpdate(kTestUpdateTask, [testScenario, game] {
		const float deltaTime = game->GetLastFrameDeltaTime();
		testScenario->Update(*game, deltaTime);
	});
	testUpdate.AddDependency(Game::Tasks::BeginFrame);
	testUpdate.AddDependency(Game::Tasks::PhysicsUpdate);
	testUpdate.AddDependency(Game::Tasks::AnimationUpdate);
	testUpdate.SetName("StressTest");

	GameTaskManager& taskManager = game->GetTaskManager();
	taskManager.AddTask(testUpdate);
	taskManager.GetTask(Game::Tasks::UpdateEnd).AddDependency(kTestUpdateTask);
	taskManager.GetTask(Game::Tasks::RemoveGameObjects).AddDependency(kTestUpdateTask);

	// start running
	while (game->IsRunning())
	{
		game->RunMainThread();
	}
	
	delete testScenario;
	game->CleanUp();

	delete game;

	return 0;
}