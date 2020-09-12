#include "../Precomp.h"
#include "PoolHeaderElem.h"
#include "PoolFreelist.h"

#include <random>

namespace BenchTable
{
	struct BigBoi {
		BigBoi(int64_t a) : myA(a), myB(0) {}
		operator int64_t() const { return myA; };

		int64_t myA;
		int64_t myB;
	};
	// This is from my amd ryzen 7 3800x
	static constexpr int64_t kL1Cache = 32 * 1024; // in Bytes
	static constexpr int64_t kL2Cache = 512 * 1024; // in Bytes
	static constexpr int64_t kL3Cache = 16 * 1024 * 1024; // in Bytes
	static constexpr int64_t kMaxSize = 8 << 12;

	constexpr int64_t GetSizeToFillCacheLevel(size_t elemSize, size_t cacheSize)
	{
		// This is obviously approximate, and doesn't account for
		// pool internal per-element memory overhead
		return (cacheSize / elemSize) - 1;
	}

	static std::vector<size_t> randomIndexMap = []() {
		std::vector<size_t> vector;
		constexpr size_t size = GetSizeToFillCacheLevel(sizeof(int64_t), kL3Cache * 2);
		vector.reserve(size);
		srand(0);
		for (size_t i = 0; i < size; i++)
		{
			vector.emplace_back(rand() % size);
		}
		return vector;
	}();

	template<class T>
	static int64_t InitSparse(T& aPool,
		std::vector<typename T::Ptr>& aPtrs,
		int64_t aSize,
		float aSparcePercent)
	{
		aPool.Resize(aSize);
		for (int64_t i = 0; i < aSize; i++)
		{
			aPtrs.emplace_back(std::move(aPool.Allocate(i)));
		}
		// make it sparse
		const int64_t removedCount = static_cast<int64_t>(aSparcePercent * aSize);
		for (int64_t i = 0; i < removedCount; i++)
		{
			const size_t index = randomIndexMap[i] % aPtrs.size();
			aPtrs.erase(aPtrs.begin() + index);
		}
		return removedCount;
	}

	template<class T>
	static int64_t InitSparseForIter(T& aPool,
		std::vector<typename T::Ptr>& aPtrs,
		int64_t aSize,
		float aSparcePercent)
	{
		int64_t result = 0;
		aPool.Resize(aSize);
		const int64_t remainingCount = static_cast<int64_t>((1.f - aSparcePercent) * aSize);
		for (int64_t i = 0; i < remainingCount; i++)
		{
			const size_t index = randomIndexMap[i] % aSize;
			T::Ptr ptr = aPool.AllocateAt(index, i);
			if (ptr.IsValid())
			{
				aPtrs.emplace_back(std::move(ptr));
				result ^= i;
			}
		}
		return result;
	}

	template<class T>
	static void PoolEmptyAdd(benchmark::State& aState)
	{
		T pool;
		pool.Resize(aState.range(0));

		std::vector<typename T::Ptr> ptrs;
		ptrs.reserve(aState.range(0));

		for (auto _ : aState)
		{
			benchmark::DoNotOptimize(pool.GetInternalBuffer());
			for (int64_t i = 0; i < aState.range(0); i++)
			{
				ptrs.emplace_back(std::move(pool.Allocate(i)));
			}
			benchmark::ClobberMemory();

			aState.PauseTiming();
			ptrs.clear();
			aState.ResumeTiming();
		}
	}

	template<class T>
	static void PoolSparseAdd(benchmark::State& aState)
	{
		T pool;
		std::vector<typename T::Ptr> ptrs;
		ptrs.reserve(aState.range(0));
		const int64_t missingElements = InitSparse(pool, ptrs, aState.range(0), aState.range(1) / 100.f);

		for (auto _ : aState)
		{
			benchmark::DoNotOptimize(pool.GetInternalBuffer());
			for (int64_t i = 0; i < missingElements; i++)
			{
				ptrs.emplace_back(std::move(pool.Allocate(i)));
			}
			benchmark::ClobberMemory();

			aState.PauseTiming();
			ptrs.clear();
			aState.ResumeTiming();
		}
	}

	template<class T>
	static void PoolFullIterate(benchmark::State& aState)
	{
		T pool;
		std::vector<typename T::Ptr> ptrs;
		ptrs.reserve(aState.range(0));
		int64_t expectedRes = InitSparseForIter(pool, ptrs, aState.range(0), 0);

		for (auto _ : aState)
		{
			int64_t res = 0;
			pool.ForEach([&res](const auto& item) {
				benchmark::DoNotOptimize(res ^= item);
			});
			benchmark::ClobberMemory();
			ASSERT(res == expectedRes);
		}
	}

	template<class T>
	static void PoolSparseIterate(benchmark::State& aState)
	{
		T pool;
		std::vector<typename T::Ptr> ptrs;
		ptrs.reserve(aState.range(0));
		int64_t expectedRes = InitSparseForIter(pool, ptrs, aState.range(0), aState.range(1) / 100.f);

		for (auto _ : aState)
		{
			int64_t res = 0;
			pool.ForEach([&res](const int64_t& item) {
				benchmark::DoNotOptimize(res ^= item);
			});
			benchmark::ClobberMemory();
			ASSERT(res == expectedRes);
		}
	}

	template<class T>
	static void PoolRealloc(benchmark::State& aState)
	{
		std::vector<typename T::Ptr> ptrs;
		ptrs.reserve(aState.range(0));

		for (auto _ : aState)
		{
			T pool;
			benchmark::DoNotOptimize(pool.GetInternalBuffer());
			for (int64_t i = 0; i < aState.range(0); i++)
			{
				ptrs.emplace_back(std::move(pool.Allocate(i)));
			}
			benchmark::ClobberMemory();

			aState.PauseTiming();
			ptrs.clear();
			aState.ResumeTiming();
		}
	}

#define TESTS(T) \
	BENCHMARK_TEMPLATE(PoolEmptyAdd, PoolHeaderElem<T>)->Range(64, kMaxSize); \
	BENCHMARK_TEMPLATE(PoolEmptyAdd, PoolFreeList<T>)->Range(64, kMaxSize); \
	\
	BENCHMARK_TEMPLATE(PoolSparseAdd, PoolHeaderElem<T>)->Ranges({ { 64, kMaxSize }, { 25, 50 } }); \
	BENCHMARK_TEMPLATE(PoolSparseAdd, PoolFreeList<T>)->Ranges({ { 64, kMaxSize }, { 25, 50 } }); \
	\
	BENCHMARK_TEMPLATE(PoolRealloc, PoolHeaderElem<T>)->Range(64, kMaxSize); \
	BENCHMARK_TEMPLATE(PoolRealloc, PoolFreeList<T>)->Range(64, kMaxSize);

#define CACHE_TESTS(T) \
	BENCHMARK_TEMPLATE(PoolFullIterate, PoolHeaderElem<T>) \
		->Arg(GetSizeToFillCacheLevel(sizeof(T), kL1Cache)) \
		->Arg(GetSizeToFillCacheLevel(sizeof(T), kL2Cache)) \
		->Arg(GetSizeToFillCacheLevel(sizeof(T), kL3Cache)) \
		->Arg(GetSizeToFillCacheLevel(sizeof(T), kL3Cache * 2)); \
	BENCHMARK_TEMPLATE(PoolFullIterate, PoolFreeList<T>) \
		->Arg(GetSizeToFillCacheLevel(sizeof(T), kL1Cache)) \
		->Arg(GetSizeToFillCacheLevel(sizeof(T), kL2Cache)) \
		->Arg(GetSizeToFillCacheLevel(sizeof(T), kL3Cache)) \
		->Arg(GetSizeToFillCacheLevel(sizeof(T), kL3Cache * 2)); \
	BENCHMARK_TEMPLATE(PoolSparseIterate, PoolHeaderElem<T>) \
		->Args({ GetSizeToFillCacheLevel(sizeof(T), kL1Cache), 50 }) \
		->Args({ GetSizeToFillCacheLevel(sizeof(T), kL2Cache), 50 }) \
		->Args({ GetSizeToFillCacheLevel(sizeof(T), kL3Cache), 50 }) \
		->Args({ GetSizeToFillCacheLevel(sizeof(T), kL3Cache * 2), 50 }); \
	BENCHMARK_TEMPLATE(PoolSparseIterate, PoolFreeList<T>) \
		->Args({ GetSizeToFillCacheLevel(sizeof(T), kL1Cache), 50 }) \
		->Args({ GetSizeToFillCacheLevel(sizeof(T), kL2Cache), 50 }) \
		->Args({ GetSizeToFillCacheLevel(sizeof(T), kL3Cache), 50 }) \
		->Args({ GetSizeToFillCacheLevel(sizeof(T), kL3Cache * 2), 50 });

	TESTS(int64_t);
	TESTS(BigBoi);
	CACHE_TESTS(int64_t);
	CACHE_TESTS(BigBoi);
}