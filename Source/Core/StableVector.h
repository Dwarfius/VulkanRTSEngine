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

        Iterator begin() 
        { 
            Iterator iter{ this, 0 }; 
            if (!iter.IsValid())
            {
                iter++;
            }
            return iter;
        }
        Iterator begin() const
        {
            Iterator iter{ this, 0 };
            if (!iter.IsValid())
            {
                iter++;
            }
            return iter;
        }

        Iterator end() { return { this, PageSize }; }
        Iterator end() const { return { this, PageSize }; }

        // Note: linear complexity!
        Iterator GetIterAt(size_t anElemNum)
        {
            Iterator iter = begin();
            while (anElemNum-- > 0)
            {
                iter++;
            }
            return iter;
        }

        FreeListNode myItems[PageSize];
        size_t myFreeNode = 0;
        size_t myCount = 0;
    };

    struct PageNode
    {
        Page myPage;
        PageNode* myNext = nullptr;
    };

    // Note: A convenience stopgap to help with ParallelFor
    // Not intended to be a proper iterator implementation, so
    // before exposing to users it needs being rounded up
    struct Iterator
    {
        T& operator*()
        {
            return *myPageIter;
        }

        void operator++(int)
        {
            do
            {
                myPageIter++;
                if (myPageIter == myPageNode->myPage.end())
                {
                    myPageNode = myPageNode->myNext;
                    if (myPageNode)
                    {
                        myPageIter = myPageNode->myPage.begin();
                    }
                }
            } while (myPageNode != nullptr
                && myPageIter != myPageNode->myPage.end()
                && !myPageIter.IsValid());
        }

        bool operator==(const Iterator& aOther) const
        {
            return myPageNode == aOther.myPageNode
                && myPageIter == aOther.myPageIter;
        }

        PageNode* myPageNode;
        Page::Iterator myPageIter;
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

        PageNode* freePage = nullptr;
        for (freePage = &myStartPage;
            !freePage->myPage.HasSpace();
            freePage = freePage->myNext)
        {
        }
        
        return freePage->myPage.Allocate(std::forward<Ts>(anArgs)...);
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

    void Reserve(size_t aSize)
    {
        PageNode* pageNode = &myStartPage;
        while (pageNode->myNext)
        {
            pageNode = pageNode->myNext;
        }

        size_t allocatedSize = myCapacity;
        while (allocatedSize < aSize)
        {
            pageNode->myNext = new PageNode;
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

        DEBUG_ONLY(std::atomic<size_t> foundCount = 0;);
        // Attempt to have 2 batches per thread, so that it's easier to schedule around
        const size_t batchSize = std::max(myCount / std::thread::hardware_concurrency() / 2, 1ull);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, myCount, batchSize),
            [&](tbb::blocked_range<size_t> aRange)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");

            // Find starting iterator
            size_t index = aRange.begin();
            PageNode* node = &myStartPage;
            while (index >= node->myPage.myCount)
            {
                index -= node->myPage.myCount;
                node = node->myNext;
            }

            // Iterate over the chunk
            Iterator iter{ node, node->myPage.GetIterAt(index) };
            for (size_t i = 0; i < aRange.size(); i++)
            {
                aFunc(*iter);
                iter++;
                DEBUG_ONLY(foundCount++;);
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

        DEBUG_ONLY(std::atomic<size_t> foundCount = 0;);
        // Attempt to have 2 batches per thread, so that it's easier to schedule around
        const size_t batchSize = std::max(myCount / std::thread::hardware_concurrency() / 2, 1ull);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, myCount, batchSize),
            [&](tbb::blocked_range<size_t> aRange)
        {
            Profiler::ScopedMark mark("StableVector::ForEachBatch");

            // Find starting iterator
            size_t index = aRange.begin();
            PageNode* node = &myStartPage;
            while (index >= node->myPage.myCount)
            {
                index -= node->myPage.myCount;
                node = node->myNext;
            }

            // Iterate over the chunk
            Iterator iter{ node, node->myPage.GetIterAt(index) };
            for (size_t i = 0; i < aRange.size(); i++)
            {
                aFunc(*iter);
                iter++;
                DEBUG_ONLY(foundCount++;);
            }
        });
        ASSERT_STR(foundCount == myCount, "Weird behavior, failed to find all elements!");
    }

private:
    PageNode myStartPage;
    size_t myCount = 0;
    size_t myCapacity = PageSize;
};