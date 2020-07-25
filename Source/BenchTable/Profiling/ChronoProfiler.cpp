#include "Precomp.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>
#include <atomic>
#include "CommonTypes.h"

namespace Chrono
{
    class HighResClock
    {
    public:
        using Stamp = std::chrono::high_resolution_clock::time_point;

        static auto now() noexcept
        {
            return std::chrono::high_resolution_clock::now();
        }

        static auto diff(Stamp start, Stamp end) noexcept
        {
            return (end - start).count();
        }
    };

    using Profiler = ProfileManager<HighResClock>;
}

void ChronoMark(benchmark::State& state)
{
    using namespace Chrono;
    Profiler::GetInstance().BeginFrame();
    for (auto _ : state)
    {
        Profiler::ScopedMark mark(__func__);
    }
    Profiler::GetInstance().EndFrame();
}

void ChronoAdd(benchmark::State& state)
{
    using namespace Chrono;
    Profiler::GetInstance().BeginFrame();
    int i = 0;
    for (auto _ : state)
    {
        Profiler::ScopedMark mark(__func__);
        benchmark::DoNotOptimize(i++);
    }
    Profiler::GetInstance().EndFrame();
}

void ChronoStrings(benchmark::State& state)
{
    using namespace Chrono;
    Profiler::GetInstance().BeginFrame();
    for (auto _ : state)
    {
        Profiler::ScopedMark mark(__func__);
        std::string var("");
        benchmark::DoNotOptimize(var.data());
        var = "123456789";
        benchmark::ClobberMemory();
    }
    Profiler::GetInstance().EndFrame();
}

void ChronoForLoop(benchmark::State& state)
{
    using namespace Chrono;
    Profiler::GetInstance().BeginFrame();
    for (auto _ : state)
    {
        Profiler::ScopedMark mark(__func__);
        for (int i = 0; i < 100; i++)
        {
            std::string var("");
            benchmark::DoNotOptimize(var.data());
            var = "123456789";
            benchmark::ClobberMemory();
        }
    }
    Profiler::GetInstance().EndFrame();
}


BENCHMARK(ChronoMark);
BENCHMARK(ChronoAdd);
BENCHMARK(ChronoStrings);
BENCHMARK(ChronoForLoop);