#pragma once

#include <variant>

template<class T, size_t PageSize = 256>
class StableVector
{
    struct Page
    {
        using FreeListNode = std::variant<T, size_t>;

        Page()
        {
            for (size_t i = 0; i < PageSize; i++)
            {
                myItems[i] = i + 1;
            }
            myItems[PageSize - 1] = static_cast<size_t>(-1ll);
        }

        template<class... Ts>
        [[nodiscard]] T& Allocate(Ts&& ...anArgs)
        {
            FreeListNode& node = myItems[myFreeNode];
            myFreeNode = std::get<1>(node);
            node = T(std::forward<Ts>(anArgs)...);
            return std::get<0>(node);
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
        }

        [[nodiscard]] bool HasSpace() const
        {
            return myFreeNode != static_cast<size_t>(-1ll);
        }

        [[nodiscard]] bool Contains(const T& anItem) const
        {
            const void* itemAddr = &anItem;
            const void* pageStart = std::begin(myItems);
            const void* pageEnd = std::end(myItems);
            return pageStart <= itemAddr && itemAddr < pageEnd;
        }

        template<class TFunc>
        void ForEach(const TFunc& aFunc)
        {
            for (FreeListNode& node : myItems)
            {
                if (node.index() == 0)
                {
                    aFunc(std::get<0>(node));
                }
            }
        }

        template<class TFunc>
        void ForEach(const TFunc& aFunc) const
        {
            for (const FreeListNode& node : myItems)
            {
                if (node.index() == 0)
                {
                    aFunc(std::get<0>(node));
                }
            }
        }

        FreeListNode myItems[PageSize];
        size_t myFreeNode = 0;
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

    template<class... Ts>
    [[nodiscard]] T& Allocate(Ts&& ...anArgs)
    {
        Page* freePage = nullptr;
        for (PageNode* pageNode = &myStartPage; pageNode; pageNode = pageNode->myNext)
        {
            Page& page = pageNode->myPage;
            if (page.HasSpace())
            {
                freePage = &page;
                break;
            }
            else if (!pageNode->myNext)
            {
                pageNode->myNext = new PageNode;
                freePage = &pageNode->myNext->myPage;
                break;
            }
        }
        return freePage->Allocate(std::forward<Ts>(anArgs)...);
    }

    void Free(T& anItem)
    {
        for (PageNode* pageNode = &myStartPage; pageNode; pageNode = pageNode->myNext)
        {
            Page& page = pageNode->myPage;
            if (page.Contains(anItem))
            {
                page.Free(anItem);
                return;
            }
        }
    }

    template<class TFunc>
    void ForEach(const TFunc& aFunc)
    {
        for (PageNode* pageNode = &myStartPage; pageNode; pageNode = pageNode->myNext)
        {
            Page& page = pageNode->myPage;
            page.ForEach(aFunc);
        }
    }

    template<class TFunc>
    void ForEach(const TFunc& aFunc) const
    {
        for (PageNode* pageNode = &myStartPage; pageNode; pageNode = pageNode->myNext)
        {
            Page& page = pageNode->myPage;
            page.ForEach(aFunc);
        }
    }

private:
    PageNode myStartPage;
};