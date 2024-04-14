#include "Precomp.h"

#include "ExpStableVector.h"
#include <Core/StableVector.h>

// This set of benchmarks is here to guide the implementation of StableVector in
// the engine to try to get it as close as possible to Vector while providing 
// pointer-stability guarantee (so it's not exactly apples to apples). It also 
// provides a space for Experimental Stable Vector to try out ideas to see if it
// more can be squeezed out

namespace Impl
{
	static void SetupBenchmark(benchmark::internal::Benchmark* aBench)
	{
		aBench->RangeMultiplier(4);
		aBench->Range(64, 4096);
	}

	static void SetupParallelBenchmark(benchmark::internal::Benchmark* aBench)
	{
		aBench->RangeMultiplier(8);
		aBench->Range(512, 32768); // 32k is enough to show 2x loss
	}
}

static void AddToVector(benchmark::State& aState)
{
	for (auto _ : aState)
	{
		std::vector<uint32_t> vec;
		for (uint32_t i = 0; i < aState.range(0); i++)
		{
			vec.push_back(i);
		}
		benchmark::DoNotOptimize(vec.data());
		benchmark::ClobberMemory();
	}
}

static void AddToReservedVector(benchmark::State& aState)
{
	for (auto _ : aState)
	{
		std::vector<uint32_t> vec;
		vec.reserve(aState.range(0));
		for (uint32_t i = 0; i < aState.range(0); i++)
		{
			vec.push_back(i);
		}
		benchmark::DoNotOptimize(vec.data());
		benchmark::ClobberMemory();
	}
}

static void AddToStableVector(benchmark::State& aState)
{
	for (auto _ : aState)
	{
		StableVector<uint32_t> vec;
		uint32_t& firstElem = vec.Allocate(0);
		for (uint32_t i = 1; i < aState.range(0); i++)
		{
			[[maybe_unused]] uint32_t& newElem = vec.Allocate(i);
		}
		benchmark::DoNotOptimize(&firstElem);
		benchmark::ClobberMemory();
	}
}

static void AddToExpStableVector(benchmark::State& aState)
{
	for (auto _ : aState)
	{
		Exp::StableVector<uint32_t> vec;
		uint32_t& firstElem = vec.Allocate(0);
		vec.Reserve(aState.range(0)); // almost no benefit for test
		for (uint32_t i = 1; i < aState.range(0); i++)
		{
			[[maybe_unused]] uint32_t& newElem = vec.Allocate(i);
		}
		benchmark::DoNotOptimize(&firstElem);
		benchmark::ClobberMemory();
	}
}

BENCHMARK(AddToVector)->Apply(Impl::SetupBenchmark);
BENCHMARK(AddToReservedVector)->Apply(Impl::SetupBenchmark);
BENCHMARK(AddToStableVector)->Apply(Impl::SetupBenchmark);
BENCHMARK(AddToExpStableVector)->Apply(Impl::SetupBenchmark);

static void ForEachVector(benchmark::State& aState)
{
	std::vector<uint32_t> vec;
	vec.reserve(aState.range(0));
	for (uint32_t i = 0; i < aState.range(0); i++)
	{
		vec.push_back(i);
	}

	for (auto _ : aState)
	{
		uint32_t sum = 0;
		for (const uint32_t& elem : vec)
		{
			benchmark::DoNotOptimize(sum += elem);
		}
	}
}

static void ForEachStableVector(benchmark::State& aState)
{
	StableVector<uint32_t> vec;
	uint32_t& firstElem = vec.Allocate(0);
	for (uint32_t i = 1; i < aState.range(0); i++)
	{
		[[maybe_unused]] uint32_t& newElem = vec.Allocate(i);
	}

	for (auto _ : aState)
	{
		uint32_t sum = 0;
		vec.ForEach([&sum](uint32_t anElem) {
			benchmark::DoNotOptimize(sum += anElem);
		});
	}
}

static void ForEachExpStableVector(benchmark::State& aState)
{
	Exp::StableVector<uint32_t> vec;
	uint32_t& firstElem = vec.Allocate(0);
	for (uint32_t i = 1; i < aState.range(0); i++)
	{
		[[maybe_unused]] uint32_t& newElem = vec.Allocate(i);
	}

	for (auto _ : aState)
	{
		uint32_t sum = 0;
		vec.ForEach([&sum](uint32_t anElem) {
			benchmark::DoNotOptimize(sum += anElem);
		});
	}
}

//BENCHMARK(ForEachVector)->Apply(Impl::SetupBenchmark);
//BENCHMARK(ForEachStableVector)->Apply(Impl::SetupBenchmark);
//BENCHMARK(ForEachExpStableVector)->Apply(Impl::SetupBenchmark);

static void ParallelForEachVector(benchmark::State& aState)
{
	std::vector<uint32_t> vec;
	vec.reserve(aState.range(0));
	for (uint32_t i = 0; i < aState.range(0); i++)
	{
		vec.push_back(i);
	}

	for (auto _ : aState)
	{
		uint32_t sum = 0;
		// same as StableVector batching
		const size_t batchSize = std::max(vec.size() / std::thread::hardware_concurrency() / 2, 1ull);
		tbb::parallel_for(tbb::blocked_range<size_t>(0, vec.size(), batchSize),
			[&](tbb::blocked_range<size_t> aRange)
		{
			for (size_t i = aRange.begin(); i < aRange.end(); i++)
			{
				benchmark::DoNotOptimize(sum += vec[i]);
			}
		});
	}
}

static void ParallelForEachStableVector(benchmark::State& aState)
{
	StableVector<uint32_t> vec;
	uint32_t& firstElem = vec.Allocate(0);
	for (uint32_t i = 1; i < aState.range(0); i++)
	{
		[[maybe_unused]] uint32_t& newElem = vec.Allocate(i);
	}

	for (auto _ : aState)
	{
		uint32_t sum = 0;
		vec.ParallelForEach([&sum](uint32_t anElem) {
			benchmark::DoNotOptimize(sum += anElem);
		});
	}
}

static void ParallelForEachExpStableVector(benchmark::State& aState)
{
	Exp::StableVector<uint32_t> vec;
	uint32_t& firstElem = vec.Allocate(0);
	for (uint32_t i = 1; i < aState.range(0); i++)
	{
		[[maybe_unused]] uint32_t& newElem = vec.Allocate(i);
	}

	for (auto _ : aState)
	{
		uint32_t sum = 0;
		vec.ParallelForEach([&sum](uint32_t anElem) {
			benchmark::DoNotOptimize(sum += anElem);
		});
	}
}

//BENCHMARK(ParallelForEachVector)->Apply(Impl::SetupParallelBenchmark);
//BENCHMARK(ParallelForEachStableVector)->Apply(Impl::SetupParallelBenchmark);
//BENCHMARK(ParallelForEachExpStableVector)->Apply(Impl::SetupParallelBenchmark);