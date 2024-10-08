#pragma once

// This define controls whether QuadTreeBF has support for sparse quads.
// The benefit is that we save memory, but it comes with a drawback - upredictable
// quad filling pattern (root quad might be filler 3rd), which can cause cache thrashing.
// I haven't seen a noticeable effect in BenchTable
#define QT_SPARSE
// Assuming QuadCount is GetQuadCount(maxDepth), AKA how many quads do we need to 
// store all Ts at all levels of tree
// Non-Sparse: 
//      QuadCount * sizeof(std::vector<T>)
// Sparse(LF - load factor, how many quads, in %,do we need to fit all items): 
//      QuadCount * sizeof(uint32_t) + LF * QuadCount * sizeof(std::vector<T>)
// Solving for LF, we get LF < 83.[4]%. As long as we don't cross this barrier over
// entire lifetime(or calls to Clear), we should see memory savings.

#ifdef QT_TELEMETRY
#define QT_TELEM(x) x

#else
#define QT_TELEM(x)
#endif

// Be warned, TItem should be a trivial type, 
// as everything internally is passed around by copy
template<class TItem>
class QuadTreeBF
{
    using Quad = uint32_t;
    static constexpr Quad kInvalidInd = static_cast<Quad>(-1);

public:
    using Info = uint32_t;

    QuadTreeBF(glm::vec2 aMin, glm::vec2 aMax, uint8_t aMaxDepth)
        : myRootMin(aMin)
        , myRootMax(aMax)
        , myMaxDepth(aMaxDepth)
    {
        ASSERT_STR(glm::epsilonEqual(aMax.x - aMin.x, aMax.y - aMin.y, glm::epsilon<float>()), 
            "Expecting a square!");
    }

    template<bool CanGrow = true>
    [[nodiscard]] Info Add(glm::vec2 aMin, glm::vec2 aMax, TItem anItem)
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
#ifdef QT_SPARSE
        uint32_t& itemsIndex = myQuads[index];
        if (itemsIndex == kInvalidInd) [[unlikely]]
        {
            itemsIndex = static_cast<uint32_t>(myItems.size());
            myItems.emplace_back();
        }
#else
        const uint32_t itemsIndex = index;
#endif
        myItems[itemsIndex].push_back(anItem);
        QT_TELEM(myTelem.myItemsAccesses++);
        QT_TELEM(myTelem.myDepthAccesses++);
        return itemsIndex;
    }

    void Remove(Info anInfo, TItem anItem)
    {
        std::vector<TItem>& cache = myItems[anInfo];
        auto cacheIter = std::ranges::find(cache, anItem);
        ASSERT(cacheIter != cache.end());
        const size_t cacheSize = cache.size();
        std::swap(*cacheIter, cache[cacheSize - 1]);
        cache.resize(cacheSize - 1);
        QT_TELEM(myTelem.myItemsAccesses++);
        QT_TELEM(myTelem.myDepthAccesses++);
    }

    Info Move(glm::vec2 aMin, glm::vec2 aMax, Info anInfo, TItem anItem)
    {
        // check to see if it'll actually move
        const uint32_t index = GetIndexForQuad(aMin, aMax, myRootMin, myRootMax, myMaxDepth);
#ifdef QT_SPARSE
        if (myQuads[index] == anInfo)
#else
        if (index == anInfo)
#endif
        {
            return anInfo;
        }

        Remove(anInfo, anItem);
        // Note on false for CanGrow:
        // If we previously inserted the item, then we should've grown
        // to accomodate it for perfect quad cell already. So we can skip the test
        return Add<false>(aMin, aMax, anItem);
    }

    void Clear()
    {
#ifdef QT_SPARSE
        myQuads.clear();
#endif
        myItems.clear();
        myMinSize = std::numeric_limits<float>::max();
        myDepth = 0;
    }

    template<class TFunc>
    void Test(glm::vec2 aMin, glm::vec2 aMax, TFunc&& aFunc)
    {
        // Processes from deepest to shallowest
        // In theory, this could lead to worse performance if the tree contains
        // non-uniformly sized sizes (which prevents us to test against largest overlaps
        // at depth-0 first). But going the opposite way would cause us to jump around in
        // memory much more if everything is uniform and fits at perfect depth.
        // I need to profile both cases to see if my guess holds remit(QuadTreeBFNaive
        // has shallowest-to-deepest impl, and it is 80% slower)
        // That said, it should be possible to provide 2 implementations and let user
        // pick which strategy to use (since we can't guess which way would be better
        // without knowing the data).
        auto [index, depth] = GetIndexAndDepthForQuad(aMin, aMax, myRootMin, myRootMax, myMaxDepth);
        const uint8_t originalDepth = depth; // saving for slow path bellow

        // fast path - there's a good chance we can find something (and early out)
        // at the perfect fit level (or above)
        while (depth)
        {
            QT_TELEM(myTelem.myDepthAccesses++);
#ifdef QT_SPARSE
            const uint32_t itemsIndex = myQuads[index];
            if (itemsIndex != kInvalidInd)
            {
#else
            const uint32_t itemsIndex = index;
            {
#endif
                for (TItem item : myItems[itemsIndex])
                {
                    QT_TELEM(myTelem.myItemsAccesses++);
                    if (!aFunc(item))
                    {
                        return;
                    }
                }
            }
            // go up a level
            const uint32_t parentCount = GetQuadCount(depth);
            const uint32_t grandParentCount = GetQuadCount(depth - 1);
            index = grandParentCount + (index - parentCount) / 4;
            depth--;
        }

        // above loop skips root, so wrap it up properly
        QT_TELEM(myTelem.myDepthAccesses++);
#ifdef QT_SPARSE
        if (myQuads[0] != kInvalidInd)
        {
            for (TItem item : myItems[myQuads[0]])
#else
        {
            for (TItem item : myItems[0])
#endif
            {
                QT_TELEM(myTelem.myItemsAccesses++);
                if (!aFunc(item))
                {
                    return;
                }
            }
        }

        // slow path - if we got here either we're interested in all overlaps 
        // (aFunc returns true), or we somehow didn't find same-or-larger objects
        // to overlap with. We have to start checking for smaller ones that are on
        // lower grid levels
        if (originalDepth + 1 <= myDepth
            // don't check deeper for cases outside of root
            && aMin.x < myRootMax.x && aMin.y < myRootMax.y 
            && aMax.x > myRootMin.x && aMax.y > myRootMin.y)
        {
            // We have a bounding volume that covers multiple quads from lower levels
            // so have to go through them all
            float size = (myRootMax.x - myRootMin.x) / (1 << (originalDepth + 1));
            glm::vec2 localMin = aMin - myRootMin;
            localMin.x = glm::max(localMin.x, 0.f);
            localMin.y = glm::max(localMin.y, 0.f);
            
            glm::vec2 localMax = aMax - myRootMin;
            // subtracting minSize allows to avoid clamps/max in the bellow loop
            localMax.x = glm::min(localMax.x, myRootMax.x - myRootMin.x - myMinSize);
            localMax.y = glm::min(localMax.y, myRootMax.y - myRootMin.y - myMinSize);
            
            for (uint8_t depthInd = originalDepth + 1; depthInd <= myDepth; depthInd++)
            {
                QT_TELEM(myTelem.myDepthAccesses++);
                const glm::uvec2 min = static_cast<glm::uvec2>(localMin / size);
                const glm::uvec2 max = static_cast<glm::uvec2>(localMax / size);
                ASSERT(max.x <= std::numeric_limits<uint16_t>::max()
                    && max.y <= std::numeric_limits<uint16_t>::max());
                const uint32_t indexOffset = GetQuadCount(depthInd);
                for (uint16_t y = static_cast<uint16_t>(min.y); y <= max.y; y++)
                {
                    for(uint16_t x = static_cast<uint16_t>(min.x); x <= max.x; x++)
                    {
                        const uint32_t quadIndex = glm::bitfieldInterleave(x, y);
#ifdef QT_SPARSE
                        const uint32_t itemsIndex = myQuads[indexOffset + quadIndex];
                        if (itemsIndex == kInvalidInd)
                        {
                            continue;
                        }
#else
                        const uint32_t itemsIndex = indexOffset + quadIndex;
#endif

                        for (TItem item : myItems[itemsIndex])
                        {
                            QT_TELEM(myTelem.myItemsAccesses++);
                            if (!aFunc(item))
                            {
                                return;
                            }
                        }
                    }
                }
                size /= 2;
            }
        }
    }

    void ResizeForMinSize(float aSize)
    {
        // we need to create a Quad that will not be able to
        // fit something of aSize size into one of it's 4 Quads
        uint8_t depth = glm::min(myMaxDepth, GetDepthForSize(aSize, myRootMax.x - myRootMin.x));
        const uint32_t quadCount = GetQuadCount(depth + 1);
#ifdef QT_SPARSE
        const uint32_t oldCount = static_cast<uint32_t>(myQuads.size());
#else
        const uint32_t oldCount = static_cast<uint32_t>(myItems.size());
#endif
        if (quadCount == oldCount)
        {
            // can't grow deeper, so fall out of here
            myMinSize = 0.f; // setup to avoid checks in the future
            return;
        }

#ifdef QT_SPARSE
        myQuads.resize(quadCount, kInvalidInd);
#else
        myItems.resize(quadCount);
#endif
        myMinSize = (myRootMax.x - myRootMin.x) / (1 << depth);
        myDepth = depth;
    }

    template<class TFunc>
    void ForEachQuad(TFunc&& aFunc)
    {
        float size = myRootMax.x - myRootMin.x;
        for (uint8_t depth = 0; depth <= myMaxDepth; depth++)
        {
            const uint32_t indexStart = GetQuadCount(depth);
            const uint32_t indexEnd = GetQuadCount(depth + 1);
            for (uint32_t index = indexStart; index < indexEnd; index++)
            {
#ifdef QT_SPARSE
                const uint32_t itemsIndex = myQuads[index];
                if (itemsIndex == kInvalidInd || myItems[itemsIndex].empty())
                {
                    continue;
                }
#else
                const uint32_t itemsIndex = index;
                if(myItems[itemsIndex].empty())
                {
                    continue;
                }
#endif

                const glm::u16vec2 coords = glm::bitfieldDeinterleave(index - indexStart);
                const glm::vec2 min = myRootMin + glm::vec2{ coords.x * size, coords.y * size };
                const glm::vec2 max = myRootMin + glm::vec2{ (coords.x + 1) * size, (coords.y + 1) * size };
                aFunc(min, max, depth, myItems[itemsIndex]);
            }
            size /= 2.f;
        }
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
            ASSERT(anIndices.x <= std::numeric_limits<uint16_t>::max());
            ASSERT(anIndices.y <= std::numeric_limits<uint16_t>::max());
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

#ifdef QT_SPARSE
    std::vector<Quad> myQuads;
#endif
    std::vector<std::vector<TItem>> myItems;
    glm::vec2 myRootMin;
    glm::vec2 myRootMax;
    // Smallest size we can fit without creating new quads
    float myMinSize = std::numeric_limits<float>::max();
    uint8_t myDepth = 0;
    uint8_t myMaxDepth;
#ifdef QT_TELEMETRY
public:
    struct Telemetry
    {
        uint32_t myItemsAccesses = 0;
        uint32_t myDepthAccesses = 0;
    };
    Telemetry myTelem;
private:
#endif

public:
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

        {
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

        struct BV 
        { 
            glm::vec2 myMin, myMax; 
        };
        BV items[]
        {
#define SQUARE(pos, size) { {pos, pos}, {pos + size, pos + size} }
            // depth 0
            SQUARE(-10, 20),
            SQUARE(-5, 10),
            SQUARE(-4.9f, 9.8f),
            SQUARE(-2.5f, 5),
            SQUARE(-1.25f, 2.5f),
            SQUARE(-1.24f, 2.48f),
            SQUARE(-1.24f, 2.48f),

            // bottom left corner
            SQUARE(-4.9f, 1.25f), // max-depth - 1
            SQUARE(-4.9f, 0.625f), // max-depth

            // top right corner
            SQUARE(4.74f, 1.25f), // max-depth - 1
            SQUARE(4.74f, 0.625f) // max-depth
#undef SQUARE
        };
        constexpr uint8_t kMaxDepth = 2;
        QuadTreeBF<uint8_t> tree({ -5, -5 }, { 5, 5 }, kMaxDepth);
        QuadTreeBF<uint8_t>::Info infos[std::size(items)];
        for (uint8_t i = 0; i < std::size(items); i++)
        {
            infos[i] = tree.Add(items[i].myMin, items[i].myMax, i);
        }
        for (float y = -6; y <= 6; y += 0.5f)
        {
            for (float x = -6; x <= 6; x += 0.5f)
            {
                uint8_t found = 0;
                tree.Test({ x, y }, { x + 0.05f, y + 0.05f }, [&](uint8_t anIndex) 
                {
                    found++;
                    return true;
                });
                ASSERT(found);
            }
        }
    }
};

#undef QT_TELEM
#undef QT_SPARSE