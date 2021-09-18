#include "Precomp.h"

#include <Engine/Game.h>
#include <Engine/Systems/ImGUI/ImGUIRendering.h>
#include <Engine/Graphics/RenderPasses/FinalCompositeRenderPass.h>

#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/UniformAdapterRegister.h>

#include <Core/Resources/BinarySerializer.h>
#include <Core/Resources/JsonSerializer.h>
#include <Core/Utils.h>

#include "Editor.h"
#include "PaintingRenderPass.h"

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
	game->Init(false);

	Graphics& graphics = *game->GetGraphics();
	{
		PaintingRenderPass* pass = new PaintingRenderPass();
		graphics.AddRenderPass(pass);
		graphics.AddRenderPassDependency(ImGUIRenderPass::kId, PaintingRenderPass::kId);

		UniformAdapterRegister::GetInstance().Register<PainterAdapter>();

		Handle<Pipeline> pipeline = game->GetAssetTracker().GetOrCreate<Pipeline>(
			"TerrainPaint/TerrainPaint.ppl"
		);
		pass->SetPipeline(pipeline, graphics);
	}

	{
		DisplayRenderPass* pass = new DisplayRenderPass();
		graphics.AddRenderPass(pass);
		graphics.AddRenderPassDependency(DisplayRenderPass::kId, ImGUIRenderPass::kId);
		graphics.AddRenderPassDependency(DisplayRenderPass::kId, PaintingRenderPass::kId);

		Handle<Pipeline> pipeline = game->GetAssetTracker().GetOrCreate<Pipeline>(
			"Engine/composite.ppl"
		);
		pass->SetPipeline(graphics.GetOrCreate(pipeline).Get<GPUPipeline>());
	}

	graphics.AddNamedFrameBuffer(
		PaintingFrameBuffer::kName, 
		PaintingFrameBuffer::kDescriptor
	);
	graphics.AddNamedFrameBuffer(
		OtherPaintingFrameBuffer::kName, 
		OtherPaintingFrameBuffer::kDescriptor
	);

	Editor* editor;
	{
		editor = new Editor(*game);
		constexpr GameTask::Type kEditorTask = Game::Tasks::Last + 1;
		GameTask task(kEditorTask, [editor]{
			editor->Run();
		});
		task.AddDependency(Game::Tasks::BeginFrame);

		GameTaskManager& taskManager = game->GetTaskManager();
		taskManager.GetTask(Game::Tasks::UpdateEnd).AddDependency(kEditorTask);
		taskManager.GetTask(Game::Tasks::RemoveGameObjects).AddDependency(kEditorTask);
		taskManager.AddTask(task);
	}

	// start running
	while (game->IsRunning())
	{
		game->RunMainThread();
	}
	
	game->CleanUp();

	delete editor;
	delete game;

	return 0;
}

