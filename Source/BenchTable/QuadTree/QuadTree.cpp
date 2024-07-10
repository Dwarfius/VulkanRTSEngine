#include "Precomp.h"

#include "QuadTreeNaive.h"

#include <Core/QuadTree.h>

struct Item
{
	glm::vec2 myMin;
	glm::vec2 myMax;
	uint64_t myInfo; // for QuadTree::Info variations
};

struct UnitTestAccess
{
	template<class T>
	static void Test()
	{
		T::UnitTest();
	}
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

template<template<class> class TQuadTree>
void QuadTreeAdd(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	UnitTestAccess::Test<Tree>();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	for (auto _ : aState)
	{
		Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
		for (Item& item : items)
		{
			[[maybe_unused]] Info info = tree.Add(item.myMin, item.myMax, &item);
		}
		ptr = &tree;
	}
}

template<template<class> class TQuadTree>
void QuadTreeAddReserved(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	UnitTestAccess::Test<Tree>();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	for (auto _ : aState)
	{
		Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
		tree.ResizeForMinSize(kMinSize);
		for (Item& item : items)
		{
			[[maybe_unused]] Info info = tree.Add<false>(item.myMin, item.myMax, &item);
		}
		ptr = &tree;
	}
}

template<template<class> class TQuadTree>
void QuadTreeTestSingle(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	UnitTestAccess::Test<Tree>();
	
	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
	tree.ResizeForMinSize(kMinSize);
	for (Item& item : items)
	{
		[[maybe_unused]] Info info = tree.Add(item.myMin, item.myMax, &item);
	}
	Item* item = &items[items.size() - 1];
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
	}
}

template<template<class> class TQuadTree>
void QuadTreeTestAll(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	UnitTestAccess::Test<Tree>();

	std::vector<Item> items = GenerateItems(aState.range(), kSize, kMinSize);
	Tree tree(glm::vec2(-kSize / 2), glm::vec2(kSize / 2), kMaxDepth);
	tree.ResizeForMinSize(kMinSize);
	for (Item& item : items)
	{
		[[maybe_unused]] Info info = tree.Add(item.myMin, item.myMax, &item);
	}
	
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
	}
}

template<template<class> class TQuadTree>
void QuadTreeMoveAll(benchmark::State& aState)
{
	using Tree = TQuadTree<Item*>;
	using Info = Tree::Info;
	UnitTestAccess::Test<Tree>();

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
	}
}

BENCHMARK(QuadTreeAdd<QuadTreeNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
BENCHMARK(QuadTreeAdd<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);

BENCHMARK(QuadTreeAddReserved<QuadTreeNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
BENCHMARK(QuadTreeAddReserved<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);

BENCHMARK(QuadTreeTestSingle<QuadTreeNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
BENCHMARK(QuadTreeTestSingle<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);

BENCHMARK(QuadTreeTestAll<QuadTreeNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);
BENCHMARK(QuadTreeTestAll<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14);

BENCHMARK(QuadTreeMoveAll<QuadTreeNaive>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);
// Note: surprisingly, 1<<17 is slightly worse than Naive's - need to think about reasons
// Reason 1: double jump to resolve quad (vs 1 for Naive) - try StableVector?
// * Nope, StableVector can potentially reduce the ::Move time, but everything
// else immediatelly degreaded to QuadTreeNaive's level - not worth
BENCHMARK(QuadTreeMoveAll<QuadTree>)->Arg(1 << 10)->Arg(1 << 12)->Arg(1 << 14)->Arg(1 << 17);