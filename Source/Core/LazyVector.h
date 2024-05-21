#pragma once

#include <vector>

// A simple utility class that only allows to insert up
// to a certain capacity, and the user must manually regrow
// the lazy vector by calling Grow().
// TItem must be default constructible - LazyVector uses
// resize() internally to allow in-place copy/move
// Inserting is O(1), Growing is O(n), Clearing is O(1),
// Random Access is O(1)
template<class TItem, size_t InitCap>
class LazyVector
{
public:
	using BaseStorage = std::vector<TItem>;
	using Iterator = typename BaseStorage::iterator;
	using ConstIterator = typename BaseStorage::const_iterator;
	constexpr static size_t kGrowthFactor = 2;

public:
	LazyVector()
		: myIndex(0)
	{
		myItems.resize(InitCap);
	}

	bool NeedsToGrow() const
	{
		return myIndex >= Capacity();
	}

	void Grow(size_t aGrowthFactor = kGrowthFactor)
	{
		myItems.resize(Capacity() * aGrowthFactor);
		Clear();
	}

	void GrowTo(size_t aNewSize)
	{
		myItems.resize(aNewSize);
		Clear();
	}

	void Clear()
	{
		ClearFrom(0);
	}

	// Inserting new items will start from anIndex
	void ClearFrom(size_t anIndex)
	{
		static_assert(std::is_trivially_constructible_v<TItem>
			&& std::is_trivially_destructible_v<TItem>, "We want to skip destroying the old sized "
			"span, as that would reuqire us to iterate over the range. We also want to skip default "
			"initializing new values after resize(since we have to have a valid object to "
			"overwrite it, or since std::vector will try to destruct all values in it's span).");
		myIndex = anIndex;
	}

	// Returns current count of elements in the vector
	size_t Size() const
	{
		return myIndex;
	}

	// Returns count of elements allocated in the vector
	size_t Capacity() const
	{
		return myItems.size();
	}

	TItem* Data()
	{
		return myItems.data();
	}

	const TItem* Data() const
	{
		return myItems.data();
	}

	TItem& operator[](size_t anIndex)
	{
		ASSERT_STR(anIndex < myIndex, "Out of bounds index!");
		return myItems[anIndex];
	}

	const TItem& operator[](size_t anIndex) const
	{
		ASSERT_STR(anIndex < myIndex, "Out of bounds index!");
		return myItems[anIndex];
	}

	// copy/move an element
	template<class T>
	bool PushBack(T&& anItem)
	{
		if (NeedsToGrow())
		{
			return false;
		}

		myItems[myIndex++] = std::forward<T>(anItem);
		return true;
	}

	// in-place construct an element
	template<class... TArgs>
	bool EmplaceBack(TArgs&&... args)
	{
		if (NeedsToGrow())
		{
			return false;
		}

		new(myItems.data() + myIndex++) TItem(std::forward<TArgs>(args)...);
		return true;
	}

	// Automatically regrows if needed
	void TransferTo(std::vector<TItem>& aVec)
	{
		const bool shouldGrow = NeedsToGrow();
		const size_t oldSize = Capacity();

		// shrink the vector to avoid returning garbage
		static_assert(std::is_trivially_destructible_v<TItem>, "Shrinking is too expensive!");
		myItems.resize(myIndex);
		aVec = std::move(myItems);
		
		// TODO: While we use std::vector internally, it'll have to allocate and
		// then value initialize the memory. Since LazyVector is supposed to help with
		// large arrays, and we require a trivial/implicit-lifetime TItem, this wastes
		// performance and becomes more expensive as LazyVector grows while having
		// low utilization. I either need to change API to be TransferTo(LazyVector&) and
		// use a custom allocator::construct that does nothing, or roll my own vector
		GrowTo(oldSize * (shouldGrow ? kGrowthFactor : 1));
	}

	// conversion move-to
	operator BaseStorage() && { return myItems; }
	// conversion move-from
	LazyVector& operator=(BaseStorage&& aVector)
	{
		myItems = std::move(aVector);
		myIndex = myItems.size();
		return *this;
	}

	// stl iterator support
	Iterator begin() noexcept
	{
		return myItems.begin();
	}

	ConstIterator begin() const noexcept
	{
		return myItems.begin();
	}

	Iterator end() noexcept
	{
		return myItems.begin() + myIndex;
	}

	ConstIterator end() const noexcept
	{
		return myItems.begin() + myIndex;
	}

private:
	BaseStorage myItems;
	size_t myIndex;
};