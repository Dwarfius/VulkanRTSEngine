#include "Precomp.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>
#include <atomic>
#include <Windows.h>
#include "CommonTypes.h"

namespace QPC
{
    class QPCClock
    {
    public:
        using Stamp = LARGE_INTEGER;
        
        static auto now() noexcept
        {
            LARGE_INTEGER stamp;
            QueryPerformanceCounter(&stamp);
            return stamp;
        }

        static auto diff(Stamp start, Stamp end) noexcept
        {
            LARGE_INTEGER diff;
            diff.QuadPart = end.QuadPart - start.QuadPart;
            
            // Yoinked from:
            // https://docs.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps?redirectedfrom=MSDN#using-qpc-in-native-code
            // We now have the elapsed number of ticks, along with the
            // number of ticks-per-second. We use these values
            // to convert to the number of elapsed nanoseconds.
            // To guard against loss-of-precision, we convert
            // to microseconds *before* dividing by ticks-per-second.
            diff.QuadPart *= 1e9;
            diff.QuadPart /= ourFrequency.QuadPart;
            return diff.QuadPart;
        }
        static LARGE_INTEGER ourFrequency;
    };
    LARGE_INTEGER QPCClock::ourFrequency = [] {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return freq;
    }();

    using Profiler = ProfileManager<QPCClock>;
}

void QPCMark(benchmark::State& state)
{
    using namespace QPC;
    Profiler::GetInstance().BeginFrame();
    for (auto _ : state)
    {
        Profiler::ScopedMark mark(__func__);
    }
    Profiler::GetInstance().EndFrame();
}

void QPCAdd(benchmark::State& state)
{
    using namespace QPC;
    Profiler::GetInstance().BeginFrame();
    int i = 0;
    for (auto _ : state)
    {
        Profiler::ScopedMark mark(__func__);
        benchmark::DoNotOptimize(i++);
    }
    Profiler::GetInstance().EndFrame();
}

void QPCStrings(benchmark::State& state)
{
    using namespace QPC;
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

void QPCForLoop(benchmark::State& state)
{
    using namespace QPC;
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

BENCHMARK(QPCMark);
BENCHMARK(QPCAdd);
BENCHMARK(QPCStrings);
BENCHMARK(QPCForLoop);