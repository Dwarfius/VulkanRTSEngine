#include "Precomp.h"

#include "EditorMode.h"

#include <Engine/Game.h>

void glfwErrorReporter(int code, const char* desc)
{
	printf("GLFW error(%d): %s", code, desc);
}

int main()
{
	// TODO: get rid of this and all rand() calls
	srand(static_cast<uint32_t>(time(0)));

	// initialize the game engine
	Game* game = new Game(&glfwErrorReporter);
	game->Init(true);

	// setup and inject our editor mode task
	EditorMode* editor = new EditorMode(*game);
	constexpr GameTask::Type kEditorUpdateTask = Game::Tasks::Last + 1;
	GameTask editorUpdate(kEditorUpdateTask, [editor, game] {
		const float deltaTime = game->GetLastFrameDeltaTime();
		PhysicsWorld* physWorld = game->GetPhysicsWorld();
		editor->Update(*game, deltaTime, physWorld);
	});
	editorUpdate.AddDependency(Game::Tasks::BeginFrame);
	editorUpdate.AddDependency(Game::Tasks::PhysicsUpdate);
	editorUpdate.AddDependency(Game::Tasks::AnimationUpdate);

	GameTaskManager& taskManager = game->GetTaskManager();
	taskManager.AddTask(editorUpdate);
	taskManager.GetTask(Game::Tasks::UpdateEnd).AddDependency(kEditorUpdateTask);
	taskManager.GetTask(Game::Tasks::RemoveGameObjects).AddDependency(kEditorUpdateTask);

	// start running
	while (game->IsRunning())
	{
		game->RunMainThread();
	}
	
	delete editor;
	game->CleanUp();

	delete game;

	return 0;
}