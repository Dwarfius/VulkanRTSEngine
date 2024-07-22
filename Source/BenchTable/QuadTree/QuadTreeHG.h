#pragma once

// This define controls whether QuadTree has support for sparse quads.
// The benefit is that we save memory, but it comes with a drawback - upredictable
// quad filling pattern (root quad might be filler 3rd), which can cause cache thrashing.
// Benchtable shows perf impact for all tests(~5%)
//#define QT_SPARSE

#ifdef QT_TELEMETRY
#define QT_TELEM(x) x
#else
#define QT_TELEM(x)
#endif

// Be warned, TItem should be a trivial type, 
// as everything internally is passed around by copy
template<class TItem>
class QuadTreeHG
{
    using Quad = uint32_t;
    static constexpr Quad kInvalidInd = static_cast<Quad>(-1);

    struct QRect
    {
        glm::u16vec2 myMin; // x,y converted to Quad coords
        // because we try to perfect fit into the grid, we can
        // shrink teh size type, since it's unlikely be too many cells
        glm::u8vec2 mySize; 
        uint8_t myDepth;

        static uint64_t Pack(QRect aRect)
        {
            const uint32_t firstHalf = aRect.myMin.x
                | static_cast<uint32_t>(aRect.myMin.y) << 16;
            const uint32_t secondHalf = aRect.mySize.x
                | static_cast<uint32_t>(aRect.mySize.y) << 8
                | static_cast<uint32_t>(aRect.myDepth) << 16;
            return firstHalf | (static_cast<uint64_t>(secondHalf) << 32);
        }

        static QRect Unpack(uint64_t anInfo)
        {
            const uint32_t firstHalf = anInfo & 0xFFFF'FFFF;
            const uint32_t secondHalf = anInfo >> 32;
            return { 
                { firstHalf & 0xFFFF, firstHalf >> 16 }, 
                { secondHalf & 0xFF, (secondHalf >> 8) & 0xFF },
                (secondHalf >> 16) & 0xFF
            };
        }
    };

public:
    using Info = uint64_t;
    static_assert(sizeof(Info) >= sizeof(QRect));

    QuadTreeHG(glm::vec2 aMin, glm::vec2 aMax, uint8_t aMaxDepth)
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
        
        const QRect rect = GetRectForQuad(aMin, aMax, myRootMin, myRootMax, myMaxDepth);
        
        const uint32_t depthOffset = GetQuadCount(rect.myDepth);
        const uint16_t sideLength = 1 << rect.myDepth;
        for (uint16_t y = 0; y <= rect.mySize.y; y++)
        {
            for (uint16_t x = 0; x <= rect.mySize.x; x++)
            {
                const uint32_t index = depthOffset +
                    (rect.myMin.y + y) * sideLength + (rect.myMin.x + x);
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
            }
        }

        // lastly, mark that particular depth level has items
        myDepthSet |= 1 << rect.myDepth;
        QT_TELEM(myTelem.myDepthAccesses++);

        return QRect::Pack(rect);
    }

    void Remove(Info anInfo, TItem anItem)
    {
        const QRect rect = QRect::Unpack(anInfo);
        const uint32_t depthOffset = GetQuadCount(rect.myDepth);
        const uint16_t sideLength = 1 << rect.myDepth;
        for (uint16_t y = 0; y <= rect.mySize.y; y++)
        {
            for (uint16_t x = 0; x <= rect.mySize.x; x++)
            {
                const uint32_t index = depthOffset + 
                    (rect.myMin.y + y) * sideLength + (rect.myMin.x + x);
#ifdef QT_SPARSE
                const uint32_t itemsIndex = myQuads[index];
#else
                const uint32_t itemsIndex = index;
#endif
                std::vector<TItem>& cache = myItems[itemsIndex];
                auto cacheIter = std::ranges::find(cache, anItem);
                ASSERT(cacheIter != cache.end());
                const size_t cacheSize = cache.size();
                std::swap(*cacheIter, cache[cacheSize - 1]);
                cache.resize(cacheSize - 1);
                QT_TELEM(myTelem.myItemsAccesses++);
            }
        }
        QT_TELEM(myTelem.myDepthAccesses++);
    }

    Info Move(glm::vec2 aMin, glm::vec2 aMax, Info anInfo, TItem anItem)
    {
        // check to see if it'll actually move
        const QRect rect = GetRectForQuad(aMin, aMax, myRootMin, myRootMax, myMaxDepth);
        if (QRect::Pack(rect) == anInfo)
        {
            // nothind to do - it would result in the same quads
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
        myDepthSet = 0;
    }

    template<class TFunc>
    void Test(glm::vec2 aMin, glm::vec2 aMax, TFunc&& aFunc)
    {
        uint16_t depthsToCheck = myDepthSet;
        uint8_t depth = 0;
        while(depthsToCheck)
        {
            if ((depthsToCheck & 1) == 0) // do we have anything at that depth level?
            {
                depth++;
                depthsToCheck >>= 1;
                continue;
            }

            QT_TELEM(myTelem.myDepthAccesses++);
            const QRect rect = GetRectForQuadAtDepth(aMin, aMax, myRootMin, myRootMax, depth);
            const uint32_t depthOffset = GetQuadCount(depth);
            const uint16_t sideLength = 1 << depth;
            for (uint16_t y = 0; y <= rect.mySize.y; y++)
            {
                for (uint16_t x = 0; x <= rect.mySize.x; x++)
                {
                    const uint32_t index = depthOffset +
                        (rect.myMin.y + y) * sideLength + (rect.myMin.x + x);
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
                }
            }

            depth++;
            depthsToCheck >>= 1;
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
    }

    template<class TFunc>
    void ForEachQuad(TFunc&& aFunc)
    {
        uint16_t depthsToCheck = myDepthSet;
        uint8_t depth = 0;
        float size = myRootMax.x - myRootMin.x;
        while (depthsToCheck)
        {
            if ((depthsToCheck & 1) == 0) // do we have anything at that depth level?
            {
                depth++;
                depthsToCheck >>= 1;
                size /= 2.f;
                continue;
            }

            const uint32_t depthOffset = GetQuadCount(depth);
            const uint16_t sideLength = 1 << depth;
            for (uint16_t y = 0; y < sideLength; y++)
            {
                for (uint16_t x = 0; x < sideLength; x++)
                {
                    const uint32_t index = depthOffset + y * sideLength + x;
#ifdef QT_SPARSE
                    const uint32_t itemsIndex = myQuads[index];
                    if (itemsIndex == kInvalidInd)
                    {
                        continue;
                    }
#else
                    const uint32_t itemsIndex = index;
#endif
                    if(myItems[itemsIndex].empty())
                    {
                        continue;
                    }

                    const glm::vec2 min = myRootMin + glm::vec2{ x * size, y * size };
                    const glm::vec2 max = myRootMin + glm::vec2{ (x + 1) * size, (y + 1) * size };
                    aFunc(min, max, depth, myItems[itemsIndex]);
                }
            }

            depth++;
            depthsToCheck >>= 1;
            size /= 2.f;
        }
    }

private:
    static QRect GetRectForQuadAtDepth(glm::vec2 aMin, glm::vec2 aMax,
        glm::vec2 aRootMin, glm::vec2 aRootMax, uint8_t aDepth)
    {
        const uint16_t sideLength = 1 << aDepth;
        const float rootSize = aRootMax.x - aRootMin.x;
        const float quadSize = rootSize / sideLength;

        auto ConvertToIndices = [](glm::uvec2 anIndices) -> glm::u16vec2
        {
            // Sanity checking we don't have values that will force us past uint32_t
            // capacity
            ASSERT(anIndices.x <= std::numeric_limits<uint16_t>::max());
            ASSERT(anIndices.y <= std::numeric_limits<uint16_t>::max());
            return static_cast<glm::u16vec2>(anIndices);
        };

        aMin = glm::clamp(aMin, aRootMin, aRootMax);
        aMax = glm::clamp(aMax, aRootMin, aRootMax);

        glm::vec2 relMin = (aMin - aRootMin) / quadSize;
        auto SanitizeMin = [](float aVal, uint16_t aBorder)
        {
            if (glm::floor(aVal) == aBorder)
            {
                aVal--;
            }
            return aVal;
        };
        relMin.x = SanitizeMin(relMin.x, sideLength);
        relMin.y = SanitizeMin(relMin.y, sideLength);
        const glm::u16vec2 minIndices = ConvertToIndices(static_cast<glm::uvec2>(relMin));

        // We need to avoid cases where it lands on the quad border, since
        // it will try either accessing outside of buffer range, or will
        // leak into a neighbor quads, making things more expensive
        auto SanitizeMax = [](float aVal, float aMin)
        {
            if (aVal >= 1.f && glm::epsilonEqual(aVal,
                glm::floor(aVal), glm::epsilon<float>()))
            {
                aVal--;
            }
            return glm::max(aVal, aMin);
        };
        glm::vec2 relMax = (aMax - aRootMin) / quadSize;
        relMax.x = SanitizeMax(relMax.x, relMin.x);
        relMax.y = SanitizeMax(relMax.y, relMin.y);
        const glm::u16vec2 maxIndices = ConvertToIndices(static_cast<glm::uvec2>(relMax));

        const glm::u16vec2 rectSize = maxIndices - minIndices;
        ASSERT_STR(rectSize.x <= std::numeric_limits<uint8_t>::max()
            && rectSize.y <= std::numeric_limits<uint8_t>::max(),
            "We're trying to save size as u8vec2, but it doesn't fit - try increasing max depth!");
        return { minIndices, static_cast<glm::u8vec2>(rectSize), aDepth };
    }

    static QRect GetRectForQuad(glm::vec2 aMin, glm::vec2 aMax, 
        glm::vec2 aRootMin, glm::vec2 aRootMax, uint8_t aMaxDepth)
    {
        // preserve original size before we clamp the imputs
        const glm::vec2 size = aMax - aMin;
        const float maxSize = glm::max(size.x, size.y);

        // we can get the depth at which we won't fit anymore
        auto GetDepthAndSizeForQuad = [aMaxDepth](float aSize, float aRootSize)
        {
            // This is just inlined GetDepthForSize that also returns resulting size
            uint8_t depth = 0;
            while (aRootSize > aSize
                && depth < aMaxDepth)
            {
                aRootSize /= 2.f;
                depth++;
            }
            return std::pair{ depth, aRootSize };
        };
        
        const float rootSize = aRootMax.x - aRootMin.x;
        const auto [depth, quadSize] = GetDepthAndSizeForQuad(maxSize, rootSize);

        return GetRectForQuadAtDepth(aMin, aMax, aRootMin, aRootMax, depth);
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
    uint16_t myDepthSet = 0;
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
        {
            const QRect min{ {0, 0}, {0, 0}, 0 };
            constexpr uint16_t maxU16 = static_cast<uint16_t>(std::numeric_limits<uint16_t>::max());
            constexpr uint8_t maxU8 = static_cast<uint16_t>(std::numeric_limits<uint8_t>::max());
            const QRect max{ { maxU16, maxU16 }, { maxU8, maxU8 }, maxU8 };
            const QRect rnd{ { 0x0123, 0x4567 }, { 0x89, 0xAB }, 0xCD };
            
            auto Equals = [](QRect a, QRect b)
            {
                return a.myMin == b.myMin && a.mySize == b.mySize && a.myDepth == b.myDepth;
            };
            ASSERT(Equals(min, QRect::Unpack(QRect::Pack(min))));
            ASSERT(Equals(max, QRect::Unpack(QRect::Pack(max))));
            ASSERT(Equals(rnd, QRect::Unpack(QRect::Pack(rnd))));
        }

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

        {
            const glm::vec2 quadMin(0);
            const glm::vec2 quadMax(4);
            // larger things should fit in root
            constexpr static uint8_t kMaxDepth = 5;
            //ASSERT(GetIndexForQuad(quadMin, quadMax, quadMin, quadMax, kMaxDepth) == 0);

            // fitting within quad
            /*ASSERT(GetIndexForQuad({ 0.5f, 0.5f }, { 3.5f, 3.5f }, quadMin, quadMax, kMaxDepth) == 0);
            ASSERT(GetIndexForQuad({ 0.5f, 0.5f }, { 1.5f, 1.5f }, quadMin, quadMax, kMaxDepth) == 1);
            ASSERT(GetIndexForQuad({ 2.5f, 0.5f }, { 3.5f, 1.5f }, quadMin, quadMax, kMaxDepth) == 2);
            ASSERT(GetIndexForQuad({ 0.5f, 2.5f }, { 1.5f, 3.5f }, quadMin, quadMax, kMaxDepth) == 3);
            ASSERT(GetIndexForQuad({ 2.5f, 2.5f }, { 3.5f, 3.5f }, quadMin, quadMax, kMaxDepth) == 4);*/

            // crossing sub quad boundary
            /*ASSERT(GetIndexForQuad({ 1.5f, 1.5f }, { 2.5f, 2.5f }, quadMin, quadMax, kMaxDepth) == 0);
            ASSERT(GetIndexForQuad({ 0.5f, 1.5f }, { 1.5f, 2.5f }, quadMin, quadMax, kMaxDepth) == 0);
            ASSERT(GetIndexForQuad({ 1.5f, 0.5f }, { 2.5f, 1.5f }, quadMin, quadMax, kMaxDepth) == 0);
            ASSERT(GetIndexForQuad({ 2.5f, 1.5f }, { 3.5f, 2.5f }, quadMin, quadMax, kMaxDepth) == 0);
            ASSERT(GetIndexForQuad({ 1.5f, 2.5f }, { 2.5f, 3.5f }, quadMin, quadMax, kMaxDepth) == 0);*/

            // crossing quad edge
            /*ASSERT(GetIndexForQuad({ 1.75f, 1.25f }, { 2.25f, 1.75f }, quadMin, quadMax, kMaxDepth) == 0);
            ASSERT(GetIndexForQuad({ 1.75f, 3.25f }, { 2.25f, 3.75f }, quadMin, quadMax, kMaxDepth) == 0);
            ASSERT(GetIndexForQuad({ 1.25f, 1.75f }, { 1.75f, 2.25f }, quadMin, quadMax, kMaxDepth) == 0);
            ASSERT(GetIndexForQuad({ 3.25f, 1.75f }, { 3.75f, 2.25f }, quadMin, quadMax, kMaxDepth) == 0);*/

            // center overlap (worst case)
            const glm::vec2 quadCenter = quadMin + (quadMax - quadMin) / 2.f;
            for (uint8_t i = 0; i < 5; i++)
            {
                const float size = 8.f / (1 << i);
                const glm::vec2 offset(size);
                //ASSERT(GetIndexForQuad(quadCenter - offset, quadCenter + offset, quadMin, quadMax, kMaxDepth) == 0);
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
        QuadTreeHG<uint8_t> tree({ -5, -5 }, { 5, 5 }, kMaxDepth);
        QuadTreeHG<uint8_t>::Info infos[std::size(items)];
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