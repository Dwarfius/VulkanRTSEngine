#pragma once

#ifdef QT_TELEMETRY
#define QT_TELEM(x) x
#else
#define QT_TELEM(x)
#endif

template<class TItem, uint32_t Capacity = 64>
class QuadTreeO
{
public:
    QT_TELEM(struct Telemetry);

    struct BV
    {
        glm::vec2 myMin, myMax;
    };

    struct Quad
    {
        BV myBV;
        uint8_t myDepth;
        uint8_t myMaxDepth;

        Quad* myQuads[4]{};
        std::vector<TItem> myItems;
        std::vector<BV> myBVs;
        QT_TELEM(Telemetry* myTelem);

        Quad(BV aBV, uint8_t aDepth, uint8_t aMaxDepth)
            : myBV(aBV)
            , myDepth(aDepth)
            , myMaxDepth(aMaxDepth)
        {
        }

        ~Quad()
        {
            Clear();
        }

        void Add(BV aBV, TItem anItem)
        {
            QT_TELEM(myTelem->myDepthAccesses++);
            if (!myQuads[0])
            {
                if (myItems.size() < Capacity
                    || myDepth == myMaxDepth)
                {
                    QT_TELEM(myTelem->myItemsAccesses++);
                    myItems.push_back(anItem);
                    myBVs.push_back(aBV);
                    return;
                }

                const glm::vec2 center = myBV.myMin + (myBV.myMax - myBV.myMin) / 2.f;
                const glm::vec2 mins[] {
                    myBV.myMin,
                    { center.x, myBV.myMin.y },
                    { myBV.myMin.x, center.y },
                    center
                };
                const glm::vec2 maxs[] {
                    center,
                    { myBV.myMax.x, center.y },
                    { center.x, myBV.myMax.y },
                    myBV.myMax
                };
                for (uint8_t i = 0; i < 4; i++)
                {
                    myQuads[i] = new Quad({ mins[i], maxs[i] }, myDepth + 1, myMaxDepth);
                    QT_TELEM(myQuads[i]->myTelem = myTelem);
                }

                for (uint16_t i = 0; i < myItems.size(); i++)
                {
                    const auto [minIndices, maxIndices] = GetMinMaxIndices(myBVs[i], myBV);
                    for (uint8_t y = minIndices.y; y <= maxIndices.y; y++)
                    {
                        for (uint8_t x = minIndices.x; x <= maxIndices.x; x++)
                        {
                            const uint8_t index = y * 2 + x;
                            myQuads[index]->Add(myBVs[i], myItems[i]);
                        }
                    }
                }
                myItems.clear();
                myBVs.clear();
            }

            const auto [minIndices, maxIndices] = GetMinMaxIndices(aBV, myBV);
            for (uint8_t y = minIndices.y; y <= maxIndices.y; y++)
            {
                for (uint8_t x = minIndices.x; x <= maxIndices.x; x++)
                {
                    const uint8_t index = y * 2 + x;
                    myQuads[index]->Add(aBV, anItem);
                }
            }
        }

        void Remove(BV aBV, TItem anItem)
        {
            QT_TELEM(myTelem->myDepthAccesses++);
            if (myQuads[0])
            {
                const auto [minIndices, maxIndices] = GetMinMaxIndices(aBV, myBV);
                for (uint8_t y = minIndices.y; y <= maxIndices.y; y++)
                {
                    for (uint8_t x = minIndices.x; x <= maxIndices.x; x++)
                    {
                        const uint8_t index = y * 2 + x;
                        myQuads[index]->Remove(aBV, anItem);
                    }
                }
            }
            else
            {
                QT_TELEM(myTelem->myItemsAccesses++);
                auto iter = std::ranges::find(myItems, anItem);
                ASSERT(iter != myItems.end());
                // we shouldn't have more than 65k items per quad
                static_assert(std::random_access_iterator<decltype(iter)>, "Bellow is not O(1)!");
                const uint16_t index = static_cast<uint16_t>(
                    std::distance(myItems.begin(), iter)
                );
                const uint16_t size = static_cast<uint16_t>(myItems.size());
                std::swap(myItems[index], myItems[size - 1]);
                myItems.resize(size - 1);
                std::swap(myBVs[index], myBVs[size - 1]);
                myBVs.resize(size - 1);
            }
        }

        void Clear()
        {
            myItems.clear();
            myBVs.clear();
            for (Quad*& quad : myQuads)
            {
                if (quad)
                {
                    delete quad; // recurses
                    quad = nullptr;
                }
            }
        }

        template<class TFunc>
        void Test(BV aBV, TFunc&& aFunc)
        {
            QT_TELEM(myTelem->myDepthAccesses++);
            if (!myItems.empty())
            {
                for (TItem item : myItems)
                {
                    QT_TELEM(myTelem->myItemsAccesses++);
                    if (!aFunc(item))
                    {
                        return;
                    }
                }
            }
            else
            {
                const auto [minIndices, maxIndices] = GetMinMaxIndices(aBV, myBV);
                for (uint8_t y = minIndices.y; y <= maxIndices.y; y++)
                {
                    for (uint8_t x = minIndices.x; x <= maxIndices.x; x++)
                    {
                        const uint8_t index = y * 2 + x;
                        myQuads[index]->Test(aBV, std::forward<TFunc>(aFunc));
                    }
                }
            }
        }

        static std::pair<glm::i8vec2, glm::i8vec2> GetMinMaxIndices(BV anItemBV, BV aQuadBV)
        {
            const glm::vec2 center = aQuadBV.myMin + (aQuadBV.myMax - aQuadBV.myMin) / 2.f;

            const glm::vec2 minDiff = anItemBV.myMin - center;
            const glm::vec2 maxDiff = anItemBV.myMax - center;

            const glm::i8vec2 minSigns = glm::sign(minDiff);
            const glm::i8vec2 maxSigns = glm::sign(maxDiff);
            // annoyingly can't figure out how to remap [-1,1] to [0, 1],
            // so just going to take a shortcut
            const uint8_t indexLUT[]{ 0, 0, 1 };
            const glm::i8vec2 minIndices{
                indexLUT[minSigns.x + 1],
                indexLUT[minSigns.y + 1]
            };
            const glm::i8vec2 maxIndices{
                indexLUT[maxSigns.x + 1],
                indexLUT[maxSigns.y + 1]
            };
            return { minIndices, maxIndices };
        }
    };

public:
    QuadTreeO(BV aRootBV, uint8_t aMaxDepth)
        : myRootQuad(aRootBV, 0, aMaxDepth)
    {
        QT_TELEM(myRootQuad.myTelem = &myTelem);
    }

    void Add(BV aBV, TItem anItem)
    {
        myRootQuad.Add(aBV, anItem);
    }

    void Remove(BV aBV, TItem anItem)
    {
        myRootQuad.Remove(aBV, anItem);
    }

    void Move(BV anOldBV, BV aNewBV, TItem anItem)
    {
        Remove(anOldBV, anItem);
        Add(aNewBV, anItem);
    }

    template<class TFunc>
    void Test(BV aBV, TFunc&& aFunc)
    {
        myRootQuad.Test(aBV, std::forward<TFunc>(aFunc));
    }

    void Clear()
    {
        myRootQuad.Clear();
    }

    static void UnitTest()
    {
        QuadTreeO<uint8_t>::BV items[]
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
        QuadTreeO<uint8_t> tree({ { -5, -5 }, { 5, 5 } }, kMaxDepth);
        for (uint8_t i = 0; i < std::size(items); i++)
        {
            tree.Add(items[i], i);
        }
        for (float y = -6; y <= 6; y += 0.5f)
        {
            for (float x = -6; x <= 6; x += 0.5f)
            {
                uint8_t found = 0;
                tree.Test({ { x, y }, { x + 0.05f, y + 0.05f } }, [&](uint8_t anIndex)
                {
                    found++;
                    return true;
                });
                ASSERT(found);
            }
        }
    }

private:
    Quad myRootQuad;
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