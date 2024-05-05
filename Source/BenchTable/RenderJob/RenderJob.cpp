#include <Precomp.h>

#include <Graphics/RenderPassJob.h>

#include <Core/CmdBuffer.h>

#include <random>

class GPUPipeline;
class GPUModel;
class GPUTexture;
class UniformBuffer;

// Goal is to test both writing and reading a lot of render jobs
// as the engine needs to be able to write those quickly and to read & execute them
// quickly as well

namespace
{
	struct Renderable
	{
		uint16_t myPipeline;
		uint16_t myModel;
		uint16_t myTexture;
		uint16_t myUniformBuffer;
	};

	struct GameState
	{
		std::vector<Renderable> myRenderables;
		std::vector<GPUPipeline*> myPipelines;
		std::vector<GPUModel*> myModels;
		std::vector<GPUTexture*> myTextures;
		std::vector<UniformBuffer*> myUniformBuffers;
	};
	
	void Generate(GameState& aGameState)
	{
		std::mt19937 gen(1234567);

		// state of StressTest with 30 spawn rate + 300 range (with an extra 0)
		constexpr uint16_t kRenderableCount = 55000;
		constexpr uint16_t kPipelineCount = 4;
		for (uint16_t i = 0; i < kPipelineCount; i++)
		{
			uint64_t pseudoPtrVal = gen();
			aGameState.myPipelines.push_back(reinterpret_cast<GPUPipeline*>(pseudoPtrVal));
		}
		constexpr uint16_t kModelCount = 3;
		for (uint16_t i = 0; i < kModelCount; i++)
		{
			uint64_t pseudoPtrVal = gen();
			aGameState.myModels.push_back(reinterpret_cast<GPUModel*>(pseudoPtrVal));
		}
		constexpr uint16_t kTextureCount = 4;
		for (uint16_t i = 0; i < kTextureCount; i++)
		{
			uint64_t pseudoPtrVal = gen();
			aGameState.myTextures.push_back(reinterpret_cast<GPUTexture*>(pseudoPtrVal));
		}
		constexpr uint16_t kUniformBufferCount = kRenderableCount;
		aGameState.myUniformBuffers.reserve(kUniformBufferCount);
		for (uint16_t i = 0; i < kUniformBufferCount; i++)
		{
			uint64_t pseudoPtrVal = gen();
			aGameState.myUniformBuffers.push_back(reinterpret_cast<UniformBuffer*>(pseudoPtrVal));
		}

		aGameState.myRenderables.reserve(kRenderableCount);
		for (uint16_t i = 0; i < kRenderableCount; i++)
		{
			aGameState.myRenderables.push_back(Renderable{
				static_cast<uint16_t>(gen() % kPipelineCount),
				static_cast<uint16_t>(gen() % kModelCount),
				static_cast<uint16_t>(gen() % kTextureCount),
				static_cast<uint16_t>(gen() % kUniformBufferCount)
			});
		}
	}
}

static void Baseline(benchmark::State& aState)
{
	GameState gameState;
	Generate(gameState);

	for (auto _ : aState)
	{
		for (const Renderable& renderable : gameState.myRenderables)
		{
			benchmark::DoNotOptimize(renderable);
		}
	}
}
BENCHMARK(Baseline);

static void JobStruct(benchmark::State& aState)
{
	GameState gameState;
	Generate(gameState);
	StableVector<RenderJob>* renderJobs = new StableVector<RenderJob>;
	renderJobs->Reserve(gameState.myRenderables.size());

	for (auto _ : aState)
	{
		aState.PauseTiming();
		renderJobs->Clear();
		aState.ResumeTiming();

		for (const Renderable& renderable : gameState.myRenderables)
		{
			RenderJob& renderJob = renderJobs->Allocate();
			renderJob.SetModel(gameState.myModels[renderable.myModel]);
			renderJob.SetPipeline(gameState.myPipelines[renderable.myPipeline]);
			renderJob.GetTextures().PushBack(gameState.myTextures[renderable.myTexture]);
			renderJob.GetUniformSet().PushBack(gameState.myUniformBuffers[renderable.myUniformBuffer]);

			RenderJob::IndexedDrawParams drawParams;
			drawParams.myOffset = 0;
			drawParams.myCount = 1234567;
			renderJob.SetDrawParams(drawParams);
		}
	}

	delete renderJobs;
}
// This is about 681% slower than just reading the Renderables set(Baseline)
BENCHMARK(JobStruct);

namespace
{
	struct SetModelCmd
	{
		static constexpr uint8_t kId = 0;
		GPUModel* myModel;
	};

	struct SetPipelineCmd
	{
		static constexpr uint8_t kId = 1;
		GPUPipeline* myPipeline;
	};

	struct SetTextureCmd
	{
		static constexpr uint8_t kId = 2;
		uint8_t mySlot;
		GPUTexture* myTexture;
	};

	struct SetUniformBufferCmd
	{
		static constexpr uint8_t kId = 3;
		uint8_t mySlot;
		UniformBuffer* myUniformBuffer;
	};

	struct DrawIndexedCmd
	{
		static constexpr uint8_t kId = 4;
		uint32_t myOffset;
		uint32_t myCount;
	};
}

static void CmdStream(benchmark::State& aState)
{
	GameState gameState;
	Generate(gameState);

	CmdBuffer cmdBuffer;
	const uint32_t kResultSize = static_cast<uint32_t>(gameState.myRenderables.size() * (
		sizeof(SetModelCmd)
		+ sizeof(SetPipelineCmd)
		+ sizeof(SetTextureCmd)
		+ sizeof(SetUniformBufferCmd)
		+ sizeof(DrawIndexedCmd)
		+ 5
	));
	cmdBuffer.Resize(kResultSize);

	for (auto _ : aState)
	{
		aState.PauseTiming();
		cmdBuffer.Clear();
		aState.ResumeTiming();

		for (const Renderable& renderable : gameState.myRenderables)
		{
			SetPipelineCmd& pipelineCmd = cmdBuffer.Write<SetPipelineCmd>();
			pipelineCmd.myPipeline = gameState.myPipelines[renderable.myPipeline];

			SetModelCmd& modelCmd = cmdBuffer.Write<SetModelCmd>();
			modelCmd.myModel = gameState.myModels[renderable.myModel];

			SetTextureCmd& textureCmd = cmdBuffer.Write<SetTextureCmd>();
			textureCmd.mySlot = 0;
			textureCmd.myTexture = gameState.myTextures[renderable.myTexture];

			SetUniformBufferCmd& uniformBufferCmd = cmdBuffer.Write<SetUniformBufferCmd>();
			uniformBufferCmd.mySlot = 0;
			uniformBufferCmd.myUniformBuffer = gameState.myUniformBuffers[renderable.myUniformBuffer];

			DrawIndexedCmd& drawCmd = cmdBuffer.Write<DrawIndexedCmd>();
			drawCmd.myOffset = 0;
			drawCmd.myCount = 1234567;
		}
	}
}
// this is about 39% faster compared to JobStruct
BENCHMARK(CmdStream);

static void CmdStreamUnchecked(benchmark::State& aState)
{
	GameState gameState;
	Generate(gameState);

	CmdBuffer cmdBuffer;
	const uint32_t kResultSize = static_cast<uint32_t>(gameState.myRenderables.size() * (
		sizeof(SetModelCmd)
		+ sizeof(SetPipelineCmd)
		+ sizeof(SetTextureCmd)
		+ sizeof(SetUniformBufferCmd)
		+ sizeof(DrawIndexedCmd)
		+ 5
	));
	cmdBuffer.Resize(kResultSize);

	for (auto _ : aState)
	{
		aState.PauseTiming();
		cmdBuffer.Clear();
		aState.ResumeTiming();

		for (const Renderable& renderable : gameState.myRenderables)
		{
			SetPipelineCmd& pipelineCmd = cmdBuffer.Write<SetPipelineCmd, false>();
			pipelineCmd.myPipeline = gameState.myPipelines[renderable.myPipeline];

			SetModelCmd& modelCmd = cmdBuffer.Write<SetModelCmd, false>();
			modelCmd.myModel = gameState.myModels[renderable.myModel];

			SetTextureCmd& textureCmd = cmdBuffer.Write<SetTextureCmd, false>();
			textureCmd.mySlot = 0;
			textureCmd.myTexture = gameState.myTextures[renderable.myTexture];

			SetUniformBufferCmd& uniformBufferCmd = cmdBuffer.Write<SetUniformBufferCmd, false>();
			uniformBufferCmd.mySlot = 0;
			uniformBufferCmd.myUniformBuffer = gameState.myUniformBuffers[renderable.myUniformBuffer];

			DrawIndexedCmd& drawCmd = cmdBuffer.Write<DrawIndexedCmd, false>();
			drawCmd.myOffset = 0;
			drawCmd.myCount = 1234567;
		}
	}
}
// this is 65% faster compared to JobStruct
BENCHMARK(CmdStreamUnchecked);

static void CmdStreamCaching(benchmark::State& aState)
{
	GameState gameState;
	Generate(gameState);

	CmdBuffer cmdBuffer;
	const uint32_t kResultSize = static_cast<uint32_t>(gameState.myRenderables.size() * (
		sizeof(SetModelCmd)
		+ sizeof(SetPipelineCmd)
		+ sizeof(SetTextureCmd)
		+ sizeof(SetUniformBufferCmd)
		+ sizeof(DrawIndexedCmd)
		+ 5
	));
	cmdBuffer.Resize(kResultSize);

	for (auto _ : aState)
	{
		aState.PauseTiming();
		cmdBuffer.Clear();
		aState.ResumeTiming();

		GPUPipeline* oldPipeline = nullptr;
		GPUModel* oldModel = nullptr;
		GPUTexture* oldTexture[4]{ nullptr, nullptr, nullptr, nullptr };
		UniformBuffer* oldUniformBuffer[4]{ nullptr, nullptr, nullptr, nullptr };

		for (const Renderable& renderable : gameState.myRenderables)
		{
			GPUPipeline* pipeline = gameState.myPipelines[renderable.myPipeline];
			if (oldPipeline != pipeline)
			{
				cmdBuffer.Write<SetPipelineCmd>().myPipeline = pipeline;
				oldPipeline = pipeline;
			}

			GPUModel* model = gameState.myModels[renderable.myModel];
			if (oldModel != model)
			{
				cmdBuffer.Write<SetModelCmd>().myModel = model;
				oldModel = model;
			}

			GPUTexture* texture = gameState.myTextures[renderable.myTexture];
			if (oldTexture[0] != texture)
			{
				SetTextureCmd& textureCmd = cmdBuffer.Write<SetTextureCmd>();
				textureCmd.mySlot = 0;
				textureCmd.myTexture = texture;

				oldTexture[0] = texture;
			}

			UniformBuffer* uniformBuffer = gameState.myUniformBuffers[renderable.myUniformBuffer];
			if (oldUniformBuffer[0] != uniformBuffer)
			{
				SetUniformBufferCmd& uniformBufferCmd = cmdBuffer.Write<SetUniformBufferCmd>();
				uniformBufferCmd.mySlot = 0;
				uniformBufferCmd.myUniformBuffer = uniformBuffer;

				oldUniformBuffer[0] = uniformBuffer;
			}

			DrawIndexedCmd& drawCmd = cmdBuffer.Write<DrawIndexedCmd>();
			drawCmd.myOffset = 0;
			drawCmd.myCount = 1234567;
		}
	}
}
// this is about 14% saving compared to JobStruct
BENCHMARK(CmdStreamCaching);