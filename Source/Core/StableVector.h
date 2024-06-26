#pragma once

#include <variant>
#include "Profiler.h"

template<class T, size_t PageSize = 256>
class StableVector
{
public:
    class Page
    {
        friend class StableVector;
        friend struct PageNode;

        using FreeListNode = std::variant<size_t, T>;
        constexpr static uint8_t kSlotIndex = 0;
        constexpr static uint8_t kValueIndex = 1;

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

        FreeListNode myItems[PageSize];
        size_t myFreeNode = 0;
        size_t myCount = 0;

    public:
        template<class TFunc>
        void ForEach(this auto& aSelf, const TFunc& aFunc)
        {
            for (auto& node : aSelf.myItems)
            {
                if (node.index() == kValueIndex)
                {
                    aFunc(std::get<kValueIndex>(node));
                }
            }
        }

        size_t GetCount() const { return myCount; }
    };

private:
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
        PageNode* currNode = myStartPage;
        while (currNode)
        {
            PageNode* node = currNode->myNext;
            delete currNode;
            currNode = node;
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
        if (myCount > myCapacity) [[unlikely]]
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
                    page = myStartPage;
                    // Note about warning of nullptr-deref: since we haven't found it 
                    // in the latter part and we know we're not full 
                    // to capacity, we must find it in the earlier part - 
                    // so no additional checks needed
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
        for (PageNode* pageNode = myStartPage; pageNode; pageNode = pageNode->myNext)
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
        for (PageNode* pageNode = myStartPage; pageNode; pageNode = pageNode->myNext)
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

    size_t GetCount() const
    {
        return myCount;
    }

    size_t GetPageCount() const
    {
        return myCapacity / PageSize;
    }

    bool Contains(const T* anItem) const
    {
        if (IsEmpty())
        {
            return false;
        }

        for (const PageNode* pageNode = myStartPage; pageNode; pageNode = pageNode->myNext)
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
        if (aSize <= myCapacity)
        {
            return;
        }

        if (!myStartPage) [[unlikely]]
        {
            myStartPage = new PageNode(0);
            myFreePage = myStartPage;
            myCapacity = PageSize;
            return;
        }

        PageNode* pageNode = myStartPage;
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
    void ForEach(this auto& aSelf, const TFunc& aFunc)
    {
        if (aSelf.IsEmpty())
        {
            return;
        }

        for (auto* pageNode = aSelf.myStartPage; pageNode; pageNode = pageNode->myNext)
        {
            auto& page = pageNode->myPage;
            page.ForEach(aFunc);
        }
    }

    template<class TFunc>
    void ParallelForEach(this auto& aSelf, const TFunc& aFunc)
    {
        if (aSelf.IsEmpty())
        {
            return;
        }

        if (aSelf.myCapacity == PageSize)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");
            aSelf.myStartPage->myPage.ForEach(aFunc);
            return;
        }

        const size_t pageCount = aSelf.GetPageCount();
        // Attempt to have 2 batches per thread, so that it's easier to schedule around
        const size_t threadCount = std::thread::hardware_concurrency() * 2;
        const size_t batchSize = std::max((pageCount + threadCount - 1) / threadCount, 1ull);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, pageCount, batchSize),
            [&](tbb::blocked_range<size_t> aRange)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");
            size_t startPage = aRange.begin();
            auto* page = aSelf.myStartPage;
            while (startPage-- > 0)
            {
                page = page->myNext;
            }

            size_t iters = aRange.size();
            while (iters-- > 0 && page)
            {
                page->myPage.ForEach(aFunc);
                page = page->myNext;
            }
        });
    }

    template<class TFunc>
    void ForEachPage(this auto& aSelf, const TFunc& aFunc)
    {
        if (aSelf.IsEmpty())
        {
            return;
        }

        size_t pageIndex = 0;
        for (auto* pageNode = aSelf.myStartPage; pageNode; pageNode = pageNode->myNext)
        {
            aFunc(pageNode->myPage, pageIndex++);
        }
    }

    template<class TFunc>
    void ParallelForEachPage(this auto& aSelf, const TFunc& aFunc)
    {
        if (aSelf.IsEmpty())
        {
            return;
        }

        if (aSelf.myCapacity == PageSize)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");
            aFunc(aSelf.myStartPage->myPage, 0);
            return;
        }

        const size_t pageCount = aSelf.GetPageCount();
        // Attempt to have 2 batches per thread, so that it's easier to schedule around
        const size_t threadCount = std::thread::hardware_concurrency() * 2;
        const size_t batchSize = std::max((pageCount + threadCount - 1) / threadCount, 1ull);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, pageCount, batchSize),
            [&](tbb::blocked_range<size_t> aRange)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");
            size_t startPage = aRange.begin();
            auto* page = aSelf.myStartPage;
            while (startPage-- > 0)
            {
                page = page->myNext;
            }

            size_t iters = aRange.size();
            size_t pageIndex = aRange.begin();
            while (iters-- > 0 && page)
            {
                aFunc(page->myPage, pageIndex++);
                page = page->myNext;
            }
        });
    }

private:
    PageNode* myStartPage = nullptr;
    PageNode* myFreePage = nullptr;
    size_t myCount = 0;
    size_t myCapacity = 0;
};