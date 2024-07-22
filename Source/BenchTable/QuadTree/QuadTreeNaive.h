#pragma once

#ifdef QT_TELEMETRY
#define QT_TELEM(x) x
#else
#define QT_TELEM(x)
#endif

template<class TItem>
struct QuadTreeNaive
{
    struct Quad;
    using Info = Quad*;
    QT_TELEM(struct Telemetry);

    struct Quad
    {
        glm::vec2 myMin;
        glm::vec2 myMax;
        uint8_t myDepth;
        uint8_t myMaxDepth;

        Quad* myQuads[4]{};
        std::vector<TItem> myItems;
        QT_TELEM(Telemetry* myTelem);

        Quad(glm::vec2 aMin, glm::vec2 aMax, uint8_t aDepth, uint8_t aMaxDepth)
            : myMin(aMin)
            , myMax(aMax)
            , myDepth(aDepth)
            , myMaxDepth(aMaxDepth)
        {
        }

        ~Quad()
        {
            Clear();
        }

        template<bool CanGrow>
        Quad* Add(glm::vec2 aMin, glm::vec2 aMax, TItem anItem)
        {
            QT_TELEM(myTelem->myDepthAccesses++);
            if (myDepth == myMaxDepth)
            {
                QT_TELEM(myTelem->myItemsAccesses++);
                myItems.push_back(anItem);
                return this;
            }

            const glm::vec2 center = myMin + (myMax - myMin) / 2.f;

            const glm::vec2 minDiff = aMin - center;
            const glm::vec2 maxDiff = aMax - center;

            const glm::ivec2 minIndices = glm::ivec2(1) + glm::ivec2(glm::sign(minDiff));
            const glm::ivec2 maxIndices = glm::ivec2(1) + glm::ivec2(glm::sign(maxDiff));

            // annoyingly can't figure out how to remap [-1,1] to [0, 1],
            // so just going to take a shortcut
            uint8_t indexLUT[]{ 0, 0, 1 };
            const uint8_t minIndex = indexLUT[minIndices.y] * 2 + indexLUT[minIndices.x];
            const uint8_t maxIndex = indexLUT[maxIndices.y] * 2 + indexLUT[maxIndices.x];
            ASSERT(minIndex < 4 && maxIndex < 4);

            if (minIndex != maxIndex)
            {
                QT_TELEM(myTelem->myItemsAccesses++);
                myItems.push_back(anItem);
                return this;
            }
            else
            {
                if constexpr (CanGrow)
                {
                    if (!myQuads[minIndex])
                    {
                        switch (minIndex)
                        {
                        case 0:
                            myQuads[minIndex] = new Quad(myMin, center, myDepth + 1, myMaxDepth);
                            break;
                        case 1:
                            myQuads[minIndex] = new Quad({ center.x, myMin.y }, { myMax.x, center.y }, myDepth + 1, myMaxDepth);
                            break;
                        case 2:
                            myQuads[minIndex] = new Quad({ myMin.x, center.y }, { center.x, myMax.y }, myDepth + 1, myMaxDepth);
                            break;
                        case 3:
                            myQuads[minIndex] = new Quad(center, myMax, myDepth + 1, myMaxDepth);
                            break;
                        }
                    }
                }
                QT_TELEM(myQuads[minIndex]->myTelem = myTelem);
                return myQuads[minIndex]->Add<CanGrow>(aMin, aMax, anItem);
            }
        }

        void Expand(uint8_t aMaxDepth)
        {
            if (myDepth == myMaxDepth)
            {
                return;
            }

            const glm::vec2 center = myMin + (myMax - myMin) / 2.f;
            for (uint8_t i = 0; i < 4; i++)
            {
                if (!myQuads[i])
                {
                    switch (i)
                    {
                    case 0:
                        myQuads[i] = new Quad(myMin, center, myDepth + 1, myMaxDepth);
                        break;
                    case 1:
                        myQuads[i] = new Quad({ center.x, myMin.y }, { myMax.x, center.y }, myDepth + 1, myMaxDepth);
                        break;
                    case 2:
                        myQuads[i] = new Quad({ myMin.x, center.y }, { center.x, myMax.y }, myDepth + 1, myMaxDepth);
                        break;
                    case 3:
                        myQuads[i] = new Quad(center, myMax, myDepth + 1, myMaxDepth);
                        break;
                    }
                }
                QT_TELEM(myQuads[i]->myTelem = myTelem);
                myQuads[i]->Expand(aMaxDepth);
            }
        }

        void Clear()
        {
            for (Quad*& quad : myQuads)
            {
                if (quad)
                {
                    quad->Clear();
                    delete quad;
                    quad = nullptr;
                }
            }
        }

        template<class TFunc>
        void Test(glm::vec2 aMin, glm::vec2 aMax, TFunc&& aFunc)
        {
            for (TItem item : myItems)
            {
                if (!aFunc(item))
                {
                    return;
                }
            }

            if (myDepth == myMaxDepth)
            {
                return;
            }

            const glm::vec2 center = myMin + (myMax - myMin) / 2.f;
            const glm::vec2 minDiff = aMin - center;
            const glm::vec2 maxDiff = aMax - center;

            const glm::ivec2 minIndices = glm::ivec2(1) + glm::ivec2(glm::sign(minDiff));
            const glm::ivec2 maxIndices = glm::ivec2(1) + glm::ivec2(glm::sign(maxDiff));

            // annoyingly can't figure out how to remap [-1,1] to [0, 1],
            // so just going to take a shortcut
            uint8_t indexLUT[]{ 0, 0, 1 };
            const uint8_t minIndex = indexLUT[minIndices.y] * 2 + indexLUT[minIndices.x];
            const uint8_t maxIndex = indexLUT[maxIndices.y] * 2 + indexLUT[maxIndices.x];
            ASSERT(minIndex < 4 && maxIndex < 4);

            QT_TELEM(myTelem->myDepthAccesses++);
            for (uint8_t index = minIndex; index <= maxIndex; index++)
            {
                if (myQuads[index])
                {
                    QT_TELEM(myTelem->myItemsAccesses++);
                    myQuads[index]->Test(aMin, aMax, std::forward<TFunc>(aFunc));
                }
            }
        }
    };

    QuadTreeNaive(glm::vec2 aMin, glm::vec2 aMax, uint8_t aMaxDepth)
        : myRoot(aMin, aMax, 0, aMaxDepth)
    {
        QT_TELEM(myRoot.myTelem = &myTelem);
    }

    template<bool CanGrow = true>
    Info Add(glm::vec2 aMin, glm::vec2 aMax, TItem anItem)
    {
        return myRoot.Add<CanGrow>(aMin, aMax, anItem);
    }

    void Remove(Info anInfo, TItem anItem)
    {
        std::vector<TItem>& cache = anInfo->myItems;
        auto cacheIter = std::ranges::find(cache, anItem);
        ASSERT(cacheIter != cache.end());
        const size_t cacheSize = cache.size();
        std::swap(*cacheIter, cache[cacheSize - 1]);
        cache.resize(cacheSize - 1);

        QT_TELEM(myTelem.myDepthAccesses++);
        QT_TELEM(myTelem.myItemsAccesses++);
    }

    Info Move(glm::vec2 aMin, glm::vec2 aMax, Info anInfo, TItem anItem)
    {
        Remove(anInfo, anItem);
        return Add(aMin, aMax, anItem);
    }

    void ResizeForMinSize(float aMinSize)
    {
        float size = myRoot.myMax.x - myRoot.myMin.x;
        uint8_t depth = 0;
        while (size > aMinSize
            && depth < myRoot.myMaxDepth)
        {
            size /= 2.f;
            depth++;
        }
        myRoot.Expand(depth);
    }

    void Clear()
    {
        myRoot.Clear();
        myRoot.myItems.clear();
    }

    template<class TFunc>
    void Test(glm::vec2 aMin, glm::vec2 aMax, TFunc&& aFunc)
    {
        myRoot.Test(aMin, aMax, std::forward<TFunc>(aFunc));
    }

    static void UnitTest()
    {

    }

    Quad myRoot;
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
};

#undef QT_TELEM