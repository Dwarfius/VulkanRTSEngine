#include "Precomp.h"

#define QT_TELEMETRY

#include "QuadTreeOriginal.h"
#include "QuadTreeBFNaive.h"
#include "QuadTreeBF.h"
#include "QuadTreeHG.h"

struct Item
{
	glm::vec2 myMin;
	glm::vec2 myMax;
	uint64_t myInfo; // for QuadTree::Info variations
};

std::vector<Item> GenerateItems(size_t aNum, float aSize, float aMinSize)
{
	std::mt19937 engine(12345);
	std::uniform_real_distribution<float> areaDistrib(-aSize / 2, aSize / 2);

	// try to reproduce a realistic scenario - we need a quad tree with aMinSize
	// to store aMinSize like objects. As a result, most of the data should be biased
	// towards aMinSize
	std::uniform_real_distribution<float> smallSizeDistrib(aMinSize, aMinSize * 2);
	std::uniform_real_distribution<float> mediumSizeDistrib(aMinSize * 2, aMinSize * 3);
	std::uniform_real_distribution<float> largeSizeDistrib(aMinSize * 3, aMinSize * 4);
	std::uniform_real_distribution<float> giantSizeDistrib(aMinSize * 4, aMinSize * 5);
	
	// layed out to generate worst case for growth
	// from largest to smallest
	constexpr float ratios[]{ 0.05f, 0.1f, 0.2f, 1.f };
	std::uniform_real_distribution<float>* distribs[]{
		&giantSizeDistrib, &largeSizeDistrib, &mediumSizeDistrib, &smallSizeDistrib
	};

	std::vector<Item> items;
	items.resize(aNum);
	size_t offset = 0;
	for (uint8_t ratioIndex = 0; ratioIndex < std::size(ratios); ratioIndex++)
	{
		const size_t count = glm::min(static_cast<size_t>(aNum * ratios[ratioIndex]), aNum - offset);
		for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
		{
			Item& item = items[itemIndex + offset];
			item.myMin.x = areaDistrib(engine);
			item.myMin.y = areaDistrib(engine);

			const glm::vec2 size((*distribs[ratioIndex])(engine), (*distribs[ratioIndex])(engine));
			item.myMax = item.myMin + size;

			item.myMax.x = glm::min(item.myMax.x, aSize / 2 - aMinSize * 0.01f);
			item.myMax.y = glm::min(item.myMax.y, aSize / 2 - aMinSize * 0.01f);
		}
		offset += count;
	}
	ASSERT(offset == aNum);
	return items;
}

constexpr float kSize = 100;
constexpr float kMinSize = 2.f;
constexpr uint8_t kMaxDepth = 5;

volatile void* ptr;

#ifdef QT_TELEMETRY
#define QT_TELEM(x) x
#else
#define QT_TELEM(x)
#endif

template<template<class> class TQuadTree>
void QuadTreeAdd(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	Tree::UnitTest();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
		for (Item& item : items)
		{
			tree.Add(item.myMin, item.myMax, &item);
		}
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		ptr = &tree;
	}
	QT_TELEM(aState.counters.insert({
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)},
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)}
	}));
}

template<template<class> class TQuadTree>
void QuadTreeAddNoInfo(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	Tree::UnitTest();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		Tree tree({ glm::vec2(-kSize / 2), glm::vec2(kSize / 2) }, kMaxDepth);
		for (Item& item : items)
		{
			tree.Add({ item.myMin, item.myMax }, &item);
		}
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		ptr = &tree;
	}
	QT_TELEM(aState.counters.insert({
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)},
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)}
		}));
}

template<template<class> class TQuadTree>
void QuadTreeAddReserved(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	Tree::UnitTest();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
		tree.ResizeForMinSize(kMinSize);
		for (Item& item : items)
		{
			[[maybe_unused]] Info info = tree.Add<false>(item.myMin, item.myMax, &item);
		}
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		ptr = &tree;
	}
	QT_TELEM(aState.counters.insert({
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)},
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)}
	}));
}

template<template<class> class TQuadTree>
void QuadTreeTestSingle(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	Tree::UnitTest();
	
	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
	tree.ResizeForMinSize(kMinSize);
	for (Item& item : items)
	{
		[[maybe_unused]] Info info = tree.Add(item.myMin, item.myMax, &item);
	}
	Item* item = &items[items.size() - 1];
	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		bool found = false;
		tree.Test(item->myMin, item->myMax, [&](Item* anItem)
		{
			if (anItem == item)
			{
				benchmark::DoNotOptimize(found = true);
			}
			return true;
		});
		ASSERT(found);
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		QT_TELEM(tree.myTelem = {});
	}
	QT_TELEM(aState.counters.insert({
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)},
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)}
	}));
}

template<template<class> class TQuadTree>
void QuadTreeTestSingleNoInfo(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	Tree::UnitTest();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	Tree tree({ glm::vec2(-kSize / 2), glm::vec2(kSize / 2) }, kMaxDepth);
	for (Item& item : items)
	{
		tree.Add({ item.myMin, item.myMax }, &item);
	}
	Item* item = &items[items.size() - 1];
	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		bool found = false;
		tree.Test({ item->myMin, item->myMax }, [&](Item* anItem)
		{
			if (anItem == item)
			{
				benchmark::DoNotOptimize(found = true);
			}
			return true;
		});
		ASSERT(found);
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		QT_TELEM(tree.myTelem = {});
	}
	QT_TELEM(aState.counters.insert({
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)},
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)}
	}));
}

template<template<class> class TQuadTree>
void QuadTreeTestAll(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	Tree::UnitTest();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
	tree.ResizeForMinSize(kMinSize);
	for (Item& item : items)
	{
		[[maybe_unused]] Info info = tree.Add(item.myMin, item.myMax, &item);
	}
	
	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		for (Item& item : items)
		{
			bool found = false;
			tree.Test(item.myMin, item.myMax, [item = &item, &found](Item* anItem)
			{
				if (anItem == item)
				{
					benchmark::DoNotOptimize(found = true);
				}
				return true;
			});
			ASSERT(found);
		}
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		QT_TELEM(tree.myTelem = {});
	}
	QT_TELEM(aState.counters.insert({
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)},
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)}
	}));
}

template<template<class> class TQuadTree>
void QuadTreeTestAllNoInfo(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	Tree::UnitTest();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	Tree tree({ glm::vec2(-kSize / 2), glm::vec2(kSize / 2) }, kMaxDepth);
	for (Item& item : items)
	{
		tree.Add({ item.myMin, item.myMax }, &item);
	}

	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		for (Item& item : items)
		{
			bool found = false;
			tree.Test({ item.myMin, item.myMax }, [item = &item, &found](Item* anItem)
			{
				if (anItem == item)
				{
					benchmark::DoNotOptimize(found = true);
				}
				return true;
			});
			ASSERT(found);
		}
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		QT_TELEM(tree.myTelem = {});
	}
	QT_TELEM(aState.counters.insert({
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)},
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)}
	}));
}

template<template<class> class TQuadTree>
void QuadTreeMoveAll(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	Tree::UnitTest();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
	tree.ResizeForMinSize(kMinSize);
	
	for (Item& item : items)
	{
		Info info = tree.Add(item.myMin, item.myMax, &item);
		std::memcpy(&item.myInfo, &info, sizeof(info));
	}

	// offset all
	constexpr glm::vec2 offsets[]{
		{5, 0},
		{-5, 0},
		{0, 5},
		{0, -5},
		{2.5f, 2.5f},
		{-2.5f, -2.5f},
		{2.5f, -2.5f},
		{-2.5f, 2.5f}
	};

	size_t iterNum = 0;
	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		aState.PauseTiming();
		const float mult = iterNum % 2 == 0 ? 1.f : -1.f;
		for (size_t i = 0; i < items.size(); i++)
		{
			const glm::vec2 offset = offsets[i % std::size(offsets)] * mult;
			items[i].myMin += offset;
			items[i].myMax += offset;
		}
		iterNum++;
		aState.ResumeTiming();
		for (Item& item : items)
		{
			Info oldInfo;
			std::memcpy(&oldInfo, &item.myInfo, sizeof(Info));
			Info info = tree.Move(item.myMin, item.myMax, oldInfo, &item);
			std::memcpy(&item.myInfo, &info, sizeof(info));
		}
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		QT_TELEM(tree.myTelem = {});
	}
	QT_TELEM(aState.counters.insert({ 
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)}, 
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)} 
	}));
}

template<template<class> class TQuadTree>
void QuadTreeMoveAllNoInfo(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	Tree::UnitTest();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	Tree tree({ glm::vec2(-kSize / 2), glm::vec2(kSize / 2) }, kMaxDepth);

	for (Item& item : items)
	{
		tree.Add({ item.myMin, item.myMax }, &item);
	}

	// offset all
	constexpr glm::vec2 offsets[]{
		{5, 0},
		{-5, 0},
		{0, 5},
		{0, -5},
		{2.5f, 2.5f},
		{-2.5f, -2.5f},
		{2.5f, -2.5f},
		{-2.5f, 2.5f}
	};

	size_t iterNum = 0;
	QT_TELEM(typename Tree::Telemetry telem);
	for (auto _ : aState)
	{
		const float mult = iterNum % 2 == 0 ? 1.f : -1.f;
		iterNum++;
		for (size_t i = 0; i < items.size(); i++)
		{
			Item& item = items[i];
			const glm::vec2 offset = offsets[i % std::size(offsets)] * mult;
			const Tree::BV oldBV{ item.myMin, item.myMax };
			item.myMin += offset;
			item.myMax += offset;
			const Tree::BV newBV{ item.myMin, item.myMax };

			tree.Move(oldBV, newBV, &item);
		}
		QT_TELEM(telem.myItemsAccesses += tree.myTelem.myItemsAccesses);
		QT_TELEM(telem.myDepthAccesses += tree.myTelem.myDepthAccesses);
		QT_TELEM(tree.myTelem = {});
	}
	QT_TELEM(aState.counters.insert({
		{"Items", benchmark::Counter(telem.myItemsAccesses, benchmark::Counter::kAvgIterations)},
		{"Depth", benchmark::Counter(telem.myDepthAccesses, benchmark::Counter::kAvgIterations)}
	}));
}
#undef QT_TELEM

#define TEST_BFNAIVE
#define TEST_BF
#define TEST_HG
#define TEST_ORIG

#ifdef TEST_BFNAIVE
BENCHMARK(QuadTreeAdd<QuadTreeBFNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif
#ifdef TEST_BF
BENCHMARK(QuadTreeAdd<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif
#ifdef TEST_HG
BENCHMARK(QuadTreeAdd<QuadTreeHG>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif
#ifdef TEST_ORIG
BENCHMARK(QuadTreeAddNoInfo<QuadTreeO>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif

#ifdef TEST_BFNAIVE
BENCHMARK(QuadTreeAddReserved<QuadTreeBFNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif
#ifdef TEST_BF
BENCHMARK(QuadTreeAddReserved<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif
#ifdef TEST_HG
BENCHMARK(QuadTreeAddReserved<QuadTreeHG>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif

#ifdef TEST_BFNAIVE
BENCHMARK(QuadTreeTestSingle<QuadTreeBFNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
#endif
#ifdef TEST_BF
BENCHMARK(QuadTreeTestSingle<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
#endif
#ifdef TEST_HG
BENCHMARK(QuadTreeTestSingle<QuadTreeHG>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
#endif
#ifdef TEST_ORIG
BENCHMARK(QuadTreeTestSingleNoInfo<QuadTreeO>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
#endif

#ifdef TEST_BFNAIVE
BENCHMARK(QuadTreeTestAll<QuadTreeBFNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
#endif
#ifdef TEST_BF
BENCHMARK(QuadTreeTestAll<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
#endif
#ifdef TEST_HG
BENCHMARK(QuadTreeTestAll<QuadTreeHG>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
#endif
#ifdef TEST_ORIG
BENCHMARK(QuadTreeTestAllNoInfo<QuadTreeO>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
#endif

#ifdef TEST_BFNAIVE
BENCHMARK(QuadTreeMoveAll<QuadTreeBFNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif
#ifdef TEST_BF
// Note: surprisingly, 1<<17 is slightly worse than Naive's - need to think about reasons
// Reason 1: double jump to resolve quad (vs 1 for Naive) - try StableVector?
// * Nope, StableVector can potentially reduce the ::Move time, but everything
// else immediatelly degreaded to QuadTreeBFNaive's level - not worth
BENCHMARK(QuadTreeMoveAll<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif
#ifdef TEST_HG
BENCHMARK(QuadTreeMoveAll<QuadTreeHG>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif
#ifdef TEST_ORIG
BENCHMARK(QuadTreeMoveAllNoInfo<QuadTreeO>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
#endif

#undef TEST_BFNAIVE
#undef TEST_BF
#undef TEST_HG
#undef TEST_ORIG