#pragma once

// Is essentially an array with a PushBack/PopBack
// functionality
template<class T, size_t N>
class StaticVector
{
public:
	using Iter = typename std::array<T, N>::iterator;
	using ConstIter = typename std::array<T, N>::const_iterator;
public:
	constexpr StaticVector() = default;

	constexpr StaticVector(std::initializer_list<T> aList)
	{
		for (const T& item : aList)
		{
			PushBack(item);
		}
	}

	constexpr void PushBack(const T& anItem)
	{
		ASSERT(myCount < N);
		myItems[myCount++] = anItem;
	}
	constexpr void PushBack(T&& anItem)
	{
		ASSERT(myCount < N);
		myItems[myCount++] = std::move(anItem);
	}

	template<class... TArgs>
	constexpr void EmplaceBack(TArgs&& ...anArgs)
	{
		ASSERT(myCount < N);
		new (&myItems[myCount++]) T(std::forward<TArgs>(anArgs)...);
	}

	constexpr void PopBack()
	{
		ASSERT(myCount > 0);
		myItems[--myCount].~T();
	}

	constexpr void Clear()
	{
		while (myCount > 0)
		{
			PopBack();
		}
	}

	constexpr size_t GetSize() const
	{
		return myCount;
	}

	// If the new size is smaller than current,
	// destroys all elements in the orphaned range.
	// If the new size is larger it does NOT construct
	// new elements
	// If the new size is the same - NOOP
	constexpr void Resize(size_t aNewSize)
	{
		ASSERT(aNewSize <= N);
		if (myCount >= aNewSize)
		{
			// Shrinking with destruction of elements
			while (myCount != aNewSize)
			{
				PopBack();
			}
		}
		else
		{
			// Growing - skipping constructing elements
			myCount = static_cast<uint16_t>(aNewSize);
		}
	}

	constexpr T& operator[](size_t anIndex)
	{
		ASSERT(anIndex < myCount);
		return myItems[anIndex];
	}

	constexpr const T& operator[](size_t anIndex) const
	{
		ASSERT(anIndex < myCount);
		return myItems[anIndex];
	}

	constexpr Iter begin()
	{
		return std::begin(myItems);
	}

	constexpr ConstIter begin() const
	{
		return std::cbegin(myItems);
	}

	constexpr Iter end()
	{
		return Iter(myItems.data(), myCount);
	}

	constexpr ConstIter end() const
	{
		return ConstIter(myItems.data(), myCount);
	}

	constexpr T* data()
	{
		return myItems.data();
	}

	constexpr const T* data() const
	{
		return myItems.data();
	}

private:
	std::array<T, N> myItems;
	uint16_t myCount = 0;
	static_assert(N < 65536, "Size too big, change type of myCount");
};