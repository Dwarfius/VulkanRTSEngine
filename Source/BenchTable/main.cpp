#include <benchmark/benchmark.h>
#include <variant>

static void TestA(benchmark::State& aState)
{
	for (auto _ : aState)
	{
		std::variant<int, float, bool> var;
	}
}

BENCHMARK(TestA);

BENCHMARK_MAIN();