#pragma once

#include <variant>
#include "Profiler.h"

template<class T, size_t PageSize = 256>
class StableVector
{
    struct Page
    {
        using FreeListNode = std::variant<size_t, T>;
        constexpr static uint8_t kSlotIndex = 0;
        constexpr static uint8_t kValueIndex = 1;

        // Note: A convenience stopgap to help with ParallelFor
        // Not intended to be a proper iterator implementation, so
        // before exposing to users it needs being rounded up
        struct Iterator
        {
            T& operator*() const
            {
                return std::get<kValueIndex>(myPage->myItems[myIndex]);
            }

            void operator++(int)
            {
                do
                {
                    myIndex++;
                } while (myIndex < PageSize
                    && !IsValid());
            }

            bool operator==(const Iterator & aOther) const
            {
                return myPage == aOther.myPage 
                    && myIndex == aOther.myIndex;
            }

            bool IsValid() const
            {
                return myPage->myItems[myIndex].index() == kValueIndex;
            }

            Page* myPage;
            size_t myIndex;
        };

        Page()
        {
            Clear();
        }

        template<class... Ts>
        [[nodiscard]] T& Allocate(Ts&& ...anArgs)
        {
            myCount++;
            FreeListNode& node = myItems[myFreeNode];
            myFreeNode = std::get<kSlotIndex>(node);
            return node.emplace<T>(std::forward<Ts>(anArgs)...);
        }

        void Free(T& anItem)
        {
            const char* const itemAddr = reinterpret_cast<const char*>(&anItem);
            const char* const pageStart = reinterpret_cast<const char*>(std::begin(myItems));
            const size_t offset = itemAddr - pageStart;
            const size_t index = offset / sizeof(FreeListNode);

            FreeListNode& node = myItems[index];
            node = myFreeNode;
            myFreeNode = index;
            myCount--;
        }

        void Clear()
        {
            for (size_t i = 0; i < PageSize; i++)
            {
                myItems[i] = i + 1;
            }
            myItems[PageSize - 1] = static_cast<size_t>(-1ll);
            myFreeNode = 0;
            myCount = 0;
        }

        [[nodiscard]] bool HasSpace() const
        {
            return myFreeNode != static_cast<size_t>(-1ll);
        }

        [[nodiscard]] bool Contains(const T* anItem) const
        {
            const void* itemAddr = anItem;
            const void* pageStart = std::begin(myItems);
            const void* pageEnd = std::end(myItems);
            return pageStart <= itemAddr && itemAddr < pageEnd;
        }

        template<class TFunc>
        void ForEach(const TFunc& aFunc)
        {
            for (FreeListNode& node : myItems)
            {
                if (node.index() == kValueIndex)
                {
                    aFunc(std::get<kValueIndex>(node));
                }
            }
        }

        template<class TFunc>
        void ForEach(const TFunc& aFunc) const
        {
            for (const FreeListNode& node : myItems)
            {
                if (node.index() == kValueIndex)
                {
                    aFunc(std::get<kValueIndex>(node));
                }
            }
        }

        FreeListNode myItems[PageSize];
        size_t myFreeNode = 0;
        size_t myCount = 0;
    };

    struct PageNode
    {
        PageNode(uint16_t anIndex) : myIndex(anIndex) {}

        Page myPage;
        PageNode* myNext = nullptr;
        uint16_t myIndex = 0;
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

    template<class... Ts>
    [[nodiscard]] T& Allocate(Ts&& ...anArgs)
    {
        myCount++; // we always allocate
        if (myCount > myCapacity)
        {
            Reserve(myCount);
        }

        T& item = myFreePage->myPage.Allocate(std::forward<Ts>(anArgs)...);
        
        // Update the cached free page
        if (!myFreePage->myPage.HasSpace()) [[unlikely]]
        {
            if (myCount == myCapacity) [[unlikely]]
            {
                // only 1 way to update the cached free page when we're full
                Reserve(myCount + 1);
            }
            else
            {
                // at this point there are only 2 cases:
                // we either have a free page later, or we have one earlier
                // since we try to always track an earlier page on Free,
                // it's likely we'll find a free page in the latter part
                PageNode* page = myFreePage;
                while (page && !page->myPage.HasSpace())
                {
                    page = page->myNext;
                }

                if (!page)
                {
                    page = &myStartPage;
                    // Note about warning of nullptr-deref: since we haven't found it 
                    // in the latter part and we know we're not full 
                    // to capacity, we must find it in the earlier part - 
                    // so no additional checks needed
#pragma warning(suppress:6011)
                    while (!page->myPage.HasSpace())
                    {
                        page = page->myNext;
                    }
                }
                myFreePage = page;
            }
        }
        
        return item;
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

                // check if we got an earlier free page
                if (pageNode->myIndex < myFreePage->myIndex)
                {
                    myFreePage = pageNode;
                }

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

    void Reserve(size_t aSize)
    {
        if (aSize < myCapacity)
        {
            return;
        }

        PageNode* pageNode = &myStartPage;
        while (pageNode->myNext)
        {
            pageNode = pageNode->myNext;
        }

        uint16_t index = pageNode->myIndex;
        size_t allocatedSize = myCapacity;

        // Unrolling 1 by hand to update the cached free page
        pageNode->myNext = new PageNode(++index);
        pageNode = pageNode->myNext;
        allocatedSize += PageSize;
        // Either the user invokes the reserve, meaning cached free page
        // is still valid (so we have to check with Unlikely assumption)
        // or we internally know we need to grow, meaning we ran out
        // of free pages
        if (!myFreePage->myPage.HasSpace()) [[unlikely]]
        {
            myFreePage = pageNode;
        }

        while (allocatedSize < aSize)
        {
            pageNode->myNext = new PageNode(++index);
            pageNode = pageNode->myNext;
            allocatedSize += PageSize;
        }
        myCapacity = allocatedSize;
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
    void ForEach(const TFunc& aFunc) const
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

        if (myCapacity == PageSize)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");
            myStartPage.myPage.ForEach(aFunc);
            return;
        }

        DEBUG_ONLY(std::atomic<size_t> foundCount = 0;);

        const size_t bucketCount = myCapacity / PageSize;
        // Attempt to have 2 batches per thread, so that it's easier to schedule around
        const size_t threadCount = std::thread::hardware_concurrency() * 2;
        const size_t batchSize = std::max((bucketCount + threadCount - 1) / threadCount, 1ull);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, bucketCount, batchSize),
            [&](tbb::blocked_range<size_t> aRange)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");
            size_t startPage = aRange.begin();
            PageNode* page = &myStartPage;
            while (startPage-- > 0)
            {
                page = page->myNext;
            }

            size_t iters = aRange.size();
            while (iters-- > 0 && page)
            {
                page->myPage.ForEach(aFunc);
                DEBUG_ONLY(foundCount += page->myPage.myCount;);
                page = page->myNext;
            }
        });
        ASSERT_STR(foundCount == myCount, "Weird behavior, failed to find all elements!");
    }

    template<class TFunc>
    void ParallelForEach(const TFunc& aFunc) const
    {
        if (IsEmpty())
        {
            return;
        }

        if (myCapacity == PageSize)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");
            myStartPage.myPage.ForEach(aFunc);
            return;
        }

        DEBUG_ONLY(std::atomic<size_t> foundCount = 0;);

        const size_t bucketCount = myCapacity / PageSize;
        // Attempt to have 2 batches per thread, so that it's easier to schedule around
        const size_t threadCount = std::thread::hardware_concurrency() * 2;
        const size_t batchSize = std::max((bucketCount + threadCount - 1) / threadCount, 1ull);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, bucketCount, batchSize),
            [&](tbb::blocked_range<size_t> aRange)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");
            size_t startPage = aRange.begin();
            PageNode* page = &myStartPage;
            while (startPage-- > 0)
            {
                page = page->myNext;
            }

            size_t iters = aRange.size();
            while (iters-- > 0 && page)
            {
                page->myPage.ForEach(aFunc);
                DEBUG_ONLY(foundCount += page->myPage.myCount;);
                page = page->myNext;
            }
        });
        ASSERT_STR(foundCount == myCount, "Weird behavior, failed to find all elements!");
    }

private:
    PageNode myStartPage{ 0 };
    PageNode* myFreePage = &myStartPage;
    size_t myCount = 0;
    size_t myCapacity = PageSize;
};