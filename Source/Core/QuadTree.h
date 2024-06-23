#pragma once

template<class TItem>
class QuadTree
{
    using Quad = uint32_t;
    static constexpr Quad kInvalidInd = static_cast<Quad>(-1);

public:
    using Info = uint32_t;

    QuadTree(glm::vec2 aMin, glm::vec2 aMax, uint8_t aMaxDepth)
        : myRootMin(aMin)
        , myRootMax(aMax)
        , myMinSize((aMax.x - aMin.x) / 2.f)
        , myMaxDepth(aMaxDepth)
    {
    }

    template<bool CanGrow = true>
    [[nodiscard]] Info Add(glm::vec2 aMin, glm::vec2 aMax, TItem* anItem)
    {
        if constexpr (CanGrow)
        {
            const glm::vec2 size = aMax - aMin;
            const float maxSize = glm::max(size.x, size.y);
            if (maxSize < myMinSize)
            {
                ResizeForMinSize(maxSize);
            }
        }
        
        const uint32_t index = GetIndexForQuad(aMin, aMax, myRootMin, myRootMax, myMaxDepth);
        uint32_t itemsIndex = myQuads[index];
        if (itemsIndex == kInvalidInd) [[unlikely]]
        {
            itemsIndex = static_cast<uint32_t>(myItems.size());
            myQuads[index] = itemsIndex;
            myItems.emplace_back();
        }
        myItems[itemsIndex].push_back(anItem);
        return itemsIndex;
    }

    void Remove(Info anInfo, TItem* anItem)
    {
        std::vector<TItem*>& cache = myItems[anInfo];
        auto cacheIter = std::ranges::find(cache, anItem);
        ASSERT(cacheIter != cache.end());
        const size_t cacheSize = cache.size();
        std::swap(*cacheIter, cache[cacheSize - 1]);
        cache.resize(cacheSize - 1);
    }

    Info Move(glm::vec2 aMin, glm::vec2 aMax, Info anInfo, TItem* anItem)
    {
        Remove(anInfo, anItem);
        // Note on false for CanGrow:
        // If we previously inserted the item, then we should've grown
        // to accomodate it for perfect quad cell already. So we can skip the test
        return Add<false>(aMin, aMax, anItem);
    }

    void Clear()
    {
        for (Quad& quad : myQuads)
        {
            quad = kInvalidInd;
        }
        myItems.clear();
        myMinSize = (myRootMax.x - myRootMin.x) / 2.f;
    }

    template<class TFunc>
    void Test(glm::vec2 aMin, glm::vec2 aMax, TFunc&& aFunc)
    {
        // Processes from deepest to shallowest
        // In theory, this could lead to worse performance if the tree contains
        // non-uniformly sized sizes (which prevents us to test against largest overlaps
        // at depth-0 first). But going the opposite way would cause us to jump around in
        // memory much more if everything is uniform and fits at perfect depth.
        // I need to profile both cases to see if my guess holds remit(QuadTreeNaive
        // has shallowest-to-deepest impl, and it is 80% slower)
        // That said, it should be possible to provide 2 implementations and let user
        // pick which strategy to use (since we can't guess which way would be better
        // without knowing the data).
        auto [index, depth] = GetIndexAndDepthForQuad(aMin, aMax, myRootMin, myRootMax, myMaxDepth);
        while (depth)
        {
            for (TItem* item : myItems[myQuads[index]])
            {
                if (!aFunc(item))
                {
                    return;
                }
            }
            // go up a level
            const uint32_t parentCount = GetQuadCount(depth);
            const uint32_t grandParentCount = GetQuadCount(depth - 1);
            index = grandParentCount + (index - parentCount) / 4;
            depth--;
        }
        // above loop skips root, so wrap it up properly
        for (TItem* item : myItems[myQuads[0]])
        {
            if (!aFunc(item))
            {
                return;
            }
        }
    }

    void ResizeForMinSize(float aSize)
    {
        // we need to create a Quad that will not be able to
        // fit something of aSize size into one of it's 4 Quads
        uint8_t depth = glm::min(myMaxDepth, GetDepthForSize(aSize, myRootMax.x - myRootMin.x));
        const uint32_t quadCount = GetQuadCount(depth + 1);
        const uint32_t oldCount = static_cast<uint32_t>(myQuads.size());
        if (quadCount == oldCount)
        {
            // can't grow deeper, so fall out of here
            myMinSize = 0.f; // setup to avoid checks in the future
            return;
        }

        myQuads.resize(quadCount, kInvalidInd);
        myMinSize = (myRootMax.x - myRootMin.x) / (1 << depth);
    }

private:
    static std::pair<uint32_t, uint8_t> GetIndexAndDepthForQuad(glm::vec2 aMin, glm::vec2 aMax, glm::vec2 aRootMin, glm::vec2 aRootMax, uint8_t aMaxDepth)
    {
        // bounds check
        if (aMin.x <= aRootMin.x || aMax.x >= aRootMax.x
            || aMin.y <= aRootMin.y || aMax.y >= aRootMax.y)
        {
            return { 0, 0 };
        }
        const glm::vec2 size = aMax - aMin;
        const float maxSize = glm::max(size.x, size.y);

        // we can get the depth at which we won't fit anymore
        auto GetDepthAndSizeForQuad = [aMaxDepth](float aSize, float aRootSize)
        {
            // This is just inlined GetDepthForSize that also returns resulting size
            uint8_t depth = 0;
            while (aRootSize > aSize
                && depth < aMaxDepth) // think this needs to be >= and depth start at 0
            {
                aRootSize /= 2.f;
                depth++;
            }
            return std::pair{ depth, aRootSize };
        };
        
        const float rootSize = aRootMax.x - aRootMin.x;
        auto [bestDepth, quadSize] = GetDepthAndSizeForQuad(maxSize, rootSize);

        auto ConvertToIndex = [](glm::uvec2 anIndices)
        {
            // For aGridSize of 4 we're working with the following indices
            // 0  1  4  5   16...
            // 2  3  6  7   18...
            // 8  9  12 13  24...
            // 10 11 14 15  26...
            // This is a Z-order curve(https://en.wikipedia.org/wiki/Z-order_curve), 
            // where cell values can be calculated by interleaving bits of X and Y coords

            // Sanity checking we don't have values that will force us past uint32_t
            // capacity
            ASSERT(anIndices.x < std::numeric_limits<uint16_t>::max());
            ASSERT(anIndices.y < std::numeric_limits<uint16_t>::max());
            const uint32_t index = glm::bitfieldInterleave(
                static_cast<uint16_t>(anIndices.x), static_cast<uint16_t>(anIndices.y)
            );
            return index;
        };
        
        const glm::vec2 relMin = aMin - aRootMin;
        const glm::vec2 relMax = aMax - aRootMin;
        do
        {
            // we're looking for a depth level where it fits within same quad
            const glm::uvec2 minIndices = static_cast<glm::uvec2>(relMin / quadSize);
            const glm::uvec2 maxIndices = static_cast<glm::uvec2>(relMax / quadSize);

            if (minIndices == maxIndices)
            {
                const uint32_t index = ConvertToIndex(minIndices);
                return { GetQuadCount(bestDepth) + index, bestDepth };
            }

            // if it doesn't fit, then we just gotta go a level higher till we
            // find where we fit
            bestDepth--;
            quadSize *= 2.f;
        } while (bestDepth);
        return { 0, 0 };
    }

    static uint32_t GetIndexForQuad(glm::vec2 aMin, glm::vec2 aMax, glm::vec2 aRootMin, glm::vec2 aRootMax, uint8_t aMaxDepth)
    {
        const auto [index, depth] = GetIndexAndDepthForQuad(aMin, aMax, aRootMin, aRootMax, aMaxDepth);
        return index;
    }

    static uint32_t GetParentIndexFromIndex(uint32_t anIndex)
    {
        ASSERT(anIndex > 0);
        const uint8_t depth = GetDepthForIndex(anIndex);
        const uint32_t parentCount = GetQuadCount(depth);
        const uint32_t grandParentCount = GetQuadCount(depth - 1);
        const uint32_t parentIndex = grandParentCount + (anIndex - parentCount) / 4;
        return parentIndex;
    }

    static uint32_t GetChildIndexFromIndex(uint32_t anIndex, uint8_t aChildInd)
    {
        const uint8_t depth = GetDepthForIndex(anIndex);
        const uint32_t start = GetQuadCount(depth + 1);
        const uint32_t offset = anIndex - GetQuadCount(depth);
        return start + offset * 4 + aChildInd;
    }

    static uint8_t GetDepthForIndex(uint32_t anIndex)
    {
        // TODO(optim): can we figure out a formula?
        // Or just check for bit count?
        uint8_t depth = 0;
        uint32_t mult = 1;
        while(anIndex >= mult)
        {
            anIndex -= mult;
            mult *= 4;
            depth++;
        }
        return depth;
    }

    // Returns the count of all quads before the given depth start
    static uint32_t GetQuadCount(uint8_t aDepth)
    {
        return ((1 << 2 * aDepth) - 1) / (4 - 1);
    }

    static uint8_t GetDepthForSize(float aSize, float aRootSize)
    {
        uint8_t depth = 0;
        while (aRootSize > aSize)
        {
            aRootSize /= 2.f;
            depth++;
        } 
        return depth;
    }

    std::vector<Quad> myQuads;
    std::vector<std::vector<TItem*>> myItems;
    glm::vec2 myRootMin;
    glm::vec2 myRootMax;
    // Smallest size we can fit without creating new quads
    float myMinSize;
    uint8_t myMaxDepth;

    friend struct UnitTestAccess;
    static void UnitTest()
    {
        ASSERT(GetQuadCount(0) == 0);
        ASSERT(GetQuadCount(1) == 1);
        ASSERT(GetQuadCount(2) == 5);
        ASSERT(GetQuadCount(3) == 21);

        ASSERT(GetDepthForIndex(0) == 0);
        ASSERT(GetDepthForIndex(1) == 1);
        ASSERT(GetDepthForIndex(2) == 1);
        ASSERT(GetDepthForIndex(3) == 1);
        ASSERT(GetDepthForIndex(4) == 1);
        ASSERT(GetDepthForIndex(5) == 2);

        ASSERT(GetDepthForSize(5, 4) == 0); // larger things should fit in root
        ASSERT(GetDepthForSize(4, 4) == 0);
        ASSERT(GetDepthForSize(3, 4) == 1);
        ASSERT(GetDepthForSize(2, 4) == 1);
        ASSERT(GetDepthForSize(1.5f, 4) == 2);
        ASSERT(GetDepthForSize(1, 4) == 2);
        ASSERT(GetDepthForSize(0.5f, 4) == 3);

        ASSERT(GetParentIndexFromIndex(1) == 0);
        ASSERT(GetParentIndexFromIndex(2) == 0);
        ASSERT(GetParentIndexFromIndex(3) == 0);
        ASSERT(GetParentIndexFromIndex(4) == 0);
        ASSERT(GetParentIndexFromIndex(5) == 1);
        ASSERT(GetParentIndexFromIndex(6) == 1);

        ASSERT(GetChildIndexFromIndex(0, 0) == 1);
        ASSERT(GetChildIndexFromIndex(0, 1) == 2);
        ASSERT(GetChildIndexFromIndex(0, 2) == 3);
        ASSERT(GetChildIndexFromIndex(0, 3) == 4);
        ASSERT(GetChildIndexFromIndex(1, 0) == 5);
        ASSERT(GetChildIndexFromIndex(1, 1) == 6);
        ASSERT(GetChildIndexFromIndex(1, 2) == 7);
        ASSERT(GetChildIndexFromIndex(1, 3) == 8);

        const glm::vec2 quadMin(0);
        const glm::vec2 quadMax(4);
        // larger things should fit in root
        constexpr static uint8_t kMaxDepth = 5;
        ASSERT(GetIndexForQuad(quadMin, quadMax, quadMin, quadMax, kMaxDepth) == 0); 

        // fitting within quad
        ASSERT(GetIndexForQuad({ 0.5f, 0.5f }, { 3.5f, 3.5f }, quadMin, quadMax, kMaxDepth) == 0);
        ASSERT(GetIndexForQuad({ 0.5f, 0.5f }, { 1.5f, 1.5f }, quadMin, quadMax, kMaxDepth) == 1);
        ASSERT(GetIndexForQuad({ 2.5f, 0.5f }, { 3.5f, 1.5f }, quadMin, quadMax, kMaxDepth) == 2);
        ASSERT(GetIndexForQuad({ 0.5f, 2.5f }, { 1.5f, 3.5f }, quadMin, quadMax, kMaxDepth) == 3);
        ASSERT(GetIndexForQuad({ 2.5f, 2.5f }, { 3.5f, 3.5f }, quadMin, quadMax, kMaxDepth) == 4);

        // crossing sub quad boundary
        ASSERT(GetIndexForQuad({ 1.5f, 1.5f }, { 2.5f, 2.5f }, quadMin, quadMax, kMaxDepth) == 0);
        ASSERT(GetIndexForQuad({ 0.5f, 1.5f }, { 1.5f, 2.5f }, quadMin, quadMax, kMaxDepth) == 0);
        ASSERT(GetIndexForQuad({ 1.5f, 0.5f }, { 2.5f, 1.5f }, quadMin, quadMax, kMaxDepth) == 0);
        ASSERT(GetIndexForQuad({ 2.5f, 1.5f }, { 3.5f, 2.5f }, quadMin, quadMax, kMaxDepth) == 0);
        ASSERT(GetIndexForQuad({ 1.5f, 2.5f }, { 2.5f, 3.5f }, quadMin, quadMax, kMaxDepth) == 0);

        // crossing quad edge
        ASSERT(GetIndexForQuad({ 1.75f, 1.25f }, { 2.25f, 1.75f }, quadMin, quadMax, kMaxDepth) == 0);
        ASSERT(GetIndexForQuad({ 1.75f, 3.25f }, { 2.25f, 3.75f }, quadMin, quadMax, kMaxDepth) == 0);
        ASSERT(GetIndexForQuad({ 1.25f, 1.75f }, { 1.75f, 2.25f }, quadMin, quadMax, kMaxDepth) == 0);
        ASSERT(GetIndexForQuad({ 3.25f, 1.75f }, { 3.75f, 2.25f }, quadMin, quadMax, kMaxDepth) == 0);

        // center overlap (worst case)
        const glm::vec2 quadCenter = quadMin + (quadMax - quadMin) / 2.f;
        for (uint8_t i = 0; i < 5; i++)
        {
            const float size = 8.f / (1 << i);
            const glm::vec2 offset(size);
            ASSERT(GetIndexForQuad(quadCenter - offset, quadCenter + offset, quadMin, quadMax, kMaxDepth) == 0);
        }
    }
};