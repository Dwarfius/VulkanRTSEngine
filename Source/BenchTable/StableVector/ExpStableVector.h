#pragma once

#include <bit>

namespace Exp
{
    template<class T, size_t PageSize = 256>
    class StableVector
    {
        struct Chunk
        {
            // Free: bit == 0. Taken: bit == 1
            // highest bit is index 0
            size_t myFreeMask = 0;
            T myItems[64];
        };

        struct Page
        {
            ~Page()
            {
                // TODO
            }

            template<class... Ts>
            [[nodiscard]] T& Allocate(Ts&& ...anArgs)
            {
                myCount++;

                // assuming we have space
                Chunk& chunk = myChunks[myFreeChunk];
                const int freeIndex = std::countl_one(chunk.myFreeMask);
                // mark it as occupied
                chunk.myFreeMask |= 1ull << (63 - freeIndex);

                while (myFreeChunk < kChunkCount 
                    && myChunks[myFreeChunk].myFreeMask == std::numeric_limits<size_t>::max())
                {
                    myFreeChunk++;
                }

                return *std::construct_at(&chunk.myItems[freeIndex], std::forward<Ts>(anArgs)...);
            }

            void Free(T& anItem)
            {
                const char* const itemAddr = reinterpret_cast<const char*>(&anItem);
                for (Chunk& chunk : myChunks)
                {
                    const char* const itemsStart = reinterpret_cast<const char*>(
                        std::begin(chunk.myItems)
                    );
                    const char* const itemsEnd = reinterpret_cast<const char*>(
                        std::end(chunk.myItems)
                    );
                    if (itemsStart <= itemAddr && itemAddr < itemsEnd)
                    {
                        const size_t offset = itemAddr - itemsStart;
                        const size_t index = offset / sizeof(T);
                        std::destroy_at(&chunk.myItems[index]);
                        myCount--;
                        return;
                    }
                }
            }

            [[nodiscard]] bool HasSpace() const
            {
                return myCount != PageSize;
            }

            [[nodiscard]] bool Contains(const T* anItem) const
            {
                const void* itemAddr = anItem;
                // technically this includes the bitmask as well, but since
                // we don't return ptrs to them, it's okay to ignore them
                const void* pageStart = std::begin(myChunks);
                const void* pageEnd = std::end(myChunks);
                return pageStart <= itemAddr && itemAddr < pageEnd;
            }

            template<class TFunc>
            void ForEach(const TFunc& aFunc)
            {
                for (Chunk& chunk : myChunks)
                {
                    for (uint8_t i = 0; i < 64; i += 4)
                    {
                        const bool b1 = chunk.myFreeMask & (1ull << (63 - i));
                        const bool b2 = chunk.myFreeMask & (1ull << (63 - i + 1));
                        const bool b3 = chunk.myFreeMask & (1ull << (63 - i + 2));
                        const bool b4 = chunk.myFreeMask & (1ull << (63 - i + 3));

                        if (b1)
                        {
                            aFunc(chunk.myItems[i]);
                        }

                        if (b2)
                        {
                            aFunc(chunk.myItems[i + 1]);
                        }

                        if (b3)
                        {
                            aFunc(chunk.myItems[i + 2]);
                        }

                        if (b4)
                        {
                            aFunc(chunk.myItems[i + 3]);
                        }
                    }
                }
            }

            constexpr static uint8_t kChunkCount = (PageSize + 63) / 64;
            Chunk myChunks[kChunkCount];
            size_t myCount = 0;
            uint8_t myFreeChunk = 0;
        };

        struct PageNode
        {
            Page myPage;
            PageNode* myNext = nullptr;
        };

    public:
        StableVector() = default;
        ~StableVector()
        {
            PageNode* node = myStartPage.myNext;
            while (node)
            {
                PageNode* currNode = node;
                node = currNode->myNext;
                delete currNode;
            }
        }

        // Copy constructors can't be supported in principle
        StableVector(const StableVector&) = delete;
        StableVector& operator=(const StableVector&) = delete;

        // Move constructors can, but needs a slightly different impl
        // for storing first page - so not implemented for now
        StableVector(StableVector&&) = delete;
        StableVector& operator=(StableVector&&) = delete;

        void Reserve(size_t aCount)
        {
            size_t pagesNeeded = aCount / PageSize;
            PageNode* currPage = &myStartPage;
            while (pagesNeeded-- > 1)
            {
                currPage->myNext = new PageNode;
                currPage = currPage->myNext;
                myCapacity += PageSize;
            }
        }

        template<class... Ts>
        [[nodiscard]] T& Allocate(Ts&& ...anArgs)
        {
            if (myCount + 1 > myCapacity)
            {
                Reserve(myCount + 1);
            }

            while (!myFreePage->myPage.HasSpace())
            {
                myFreePage = myFreePage->myNext;
            }

            myCount++;
            return myFreePage->myPage.Allocate(std::forward<Ts>(anArgs)...);
        }

        void Free(T& anItem)
        {
            for (PageNode* pageNode = &myStartPage; pageNode; pageNode = pageNode->myNext)
            {
                Page& page = pageNode->myPage;
                if (page.Contains(&anItem))
                {
                    page.Free(anItem);
                    myCount--;
                    return;
                }
            }
        }

        void Clear()
        {
            for (PageNode* pageNode = &myStartPage; pageNode; pageNode = pageNode->myNext)
            {
                Page& page = pageNode->myPage;
                page.Clear();
            }
            myCount = 0;
        }

        bool IsEmpty() const
        {
            return myCount == 0;
        }

        bool Contains(const T* anItem) const
        {
            if (IsEmpty())
            {
                return false;
            }

            for (const PageNode* pageNode = &myStartPage; pageNode; pageNode = pageNode->myNext)
            {
                const Page& page = pageNode->myPage;
                if (page.Contains(anItem))
                {
                    return true;
                }
            }
            return false;
        }

        template<class TFunc>
        void ForEach(const TFunc& aFunc)
        {
            if (IsEmpty())
            {
                return;
            }

            for (PageNode* pageNode = &myStartPage; pageNode; pageNode = pageNode->myNext)
            {
                Page& page = pageNode->myPage;
                page.ForEach(aFunc);
            }
        }

        template<class TFunc>
        void ParallelForEach(const TFunc& aFunc)
        {
            if (IsEmpty())
            {
                return;
            }

            DEBUG_ONLY(std::atomic<size_t> foundCount = 0;);
            // Attempt to have 2 batches per thread, so that it's easier to schedule around
            const size_t batchSize = std::max(myCount / std::thread::hardware_concurrency() / 2, 1ull);
            tbb::parallel_for(tbb::blocked_range<size_t>(0, myCount, batchSize),
                [&](tbb::blocked_range<size_t> aRange)
            {
                // TODO
            });
            ASSERT_STR(foundCount == myCount, "Weird behavior, failed to find all elements!");
        }

    private:
        PageNode myStartPage;
        size_t myCount = 0;
        size_t myCapacity = PageSize;
        // TODO: This was a win! need to add handling in Free
        PageNode* myFreePage = &myStartPage;
    };
}