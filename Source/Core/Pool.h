#pragma once

#ifdef ASSERT_MUTEX
#include "Threading/AssertRWMutex.h"
#endif
#include "Profiler.h"

// A collection object that has ability to grow like a vector, 
// while guarantying pointer stability (via proxy type called Ptr).
// The purpose is to help maintain cache coherence
// It is not thread safe since an Allocate can cause a realloc,
// so dereferencing Ptr can end up in stale memory
template<class T>
class Pool
{
	using GenerationType = uint8_t;

	constexpr static uint8_t kGrowthFactor = 2;
	constexpr static uint8_t kInitialCapacity = 64;

	class PtrBase
	{
	public:
		PtrBase() = default;
		PtrBase(Pool<T>* aPool, size_t anIndex, GenerationType aGeneration)
			: myPool(aPool)
			, myIndex(anIndex)
			, myGeneration(aGeneration)
		{
		}

		bool IsValid() const;
		T* Get();
		const T* Get() const;

	protected:
		void Reset()
		{
			if (IsValid())
			{
				myPool->Free(myIndex);
			}
		}
		Pool<T>* myPool = nullptr; // not owned
		size_t myIndex = 0;
		GenerationType myGeneration = 0;
	};
public:
	// RAII style Pointer to a pool-allocated object
	// Destroys the pointed-to object on life end
	class Ptr : public PtrBase
	{
	public:
		Ptr() = default;
		Ptr(const Ptr&) = delete;
		Ptr& operator=(const Ptr&) = delete;

		Ptr(Ptr&& aPtr) noexcept { *this = std::move(aPtr); }
		Ptr& operator=(Ptr&& aPtr) noexcept;

		~Ptr() noexcept
		{
			this->Reset();
		}
	
	private:
		friend class Pool<T>;
		Ptr(Pool<T>* aPool, size_t anIndex, GenerationType aGeneration)
			: PtrBase(aPool, anIndex, aGeneration)
		{
		}
	};

	// Safe pointer to a pool-allocated object
	// Doesn't destroy pointed-to object on life end
	class WeakPtr : public PtrBase
	{
	public:
		WeakPtr() = default;
		WeakPtr(const Ptr& aPtr) : PtrBase(aPtr) {}
	};

	// Allocates a new element and returns a Ptr for it.
	// Can cause pool to resize. Ptrs will remain valid.
	template<class... TArgs>
	Ptr Allocate(TArgs&&... aConstrArgs);

	template<class TUnaryFunc> requires std::is_invocable_v<TUnaryFunc, T&>
	void ForEach(TUnaryFunc aFunc);

	template<class TUnaryFunc> requires std::is_invocable_v<TUnaryFunc, const T&>
	void ForEach(TUnaryFunc aFunc) const;

	template<class TUnaryFunc> requires std::is_invocable_v<TUnaryFunc, T&>
	void ParallelForEach(TUnaryFunc aFunc);

	template<class TUnaryFunc> requires std::is_invocable_v<TUnaryFunc, const T&>
	void ParallelForEach(TUnaryFunc aFunc) const;

	size_t GetCapacity() const { return myElements.capacity(); }
	size_t GetSize() const { return mySize; }

private:
	struct Element
	{
		std::variant<size_t, T> myDataVariant;
		GenerationType myGeneration;

		Element() noexcept
			: myDataVariant(0ull)
			, myGeneration(0)
		{
		}
	};

	void Resize(size_t aNewCount);
	void Free(size_t anIndex);
	bool IsValid(size_t anIndex, GenerationType aGeneration) const;

	std::vector<Element> myElements;
	size_t myFirstFreeSlot = 0;
	size_t mySize = 0;
#ifdef ASSERT_MUTEX
	AssertRWMutex myIterationMutex;
#endif
};

// ======================
//        PtrBase
// ======================

template<class T>
bool Pool<T>::PtrBase::IsValid() const
{
	return myPool && myPool->IsValid(myIndex, myGeneration);
}

template<class T>
T* Pool<T>::PtrBase::Get()
{
	ASSERT_STR(IsValid(), "Invalid Ptr!");
	return &std::get<1>(myPool->myElements[myIndex].myDataVariant);
}

template<class T>
const T* Pool<T>::PtrBase::Get() const
{
	ASSERT_STR(IsValid(), "Invalid Ptr!");
	return &std::get<1>(myPool->myElements[myIndex].myDataVariant);
}

// ======================
//          Ptr
// ======================

template<class T>
typename Pool<T>::Ptr& Pool<T>::Ptr::operator=(Ptr&& aPtr) noexcept
{
	this->Reset();
	this->myPool = aPtr.myPool;
	this->myIndex = aPtr.myIndex;
	this->myGeneration = aPtr.myGeneration;
	aPtr.myPool = nullptr;
	aPtr.myIndex = 0;
	aPtr.myGeneration = 0;
	return *this;
}

// ======================
//         Pool
// ======================

template<class T>
template<class... TArgs>
typename Pool<T>::Ptr Pool<T>::Allocate(TArgs&&... aConstrArgs)
{
#ifdef ASSERT_MUTEX
	// Can't allocate while iterating!
	AssertReadLock lock(myIterationMutex);
#endif

	if (mySize == myElements.capacity())
	{
		const size_t newSize = myElements.capacity() ? myElements.capacity() * kGrowthFactor : kInitialCapacity;
		Resize(newSize);
	}

	// find first free spot
	const size_t currIndex = myFirstFreeSlot;
	Element& currFreeSpot = myElements[currIndex];
	ASSERT_STR(currFreeSpot.myDataVariant.index() == 0, "Weird pool state detected!");

	// move the pointer to next free slot
	myFirstFreeSlot = std::get<0>(currFreeSpot.myDataVariant);

	// finally construct the new one
	currFreeSpot.myDataVariant.emplace<T>(std::forward<TArgs>(aConstrArgs)...);
	currFreeSpot.myGeneration = currFreeSpot.myGeneration++;
	mySize++;

	return Ptr(this, currIndex, currFreeSpot.myGeneration);
}

template<class T>
template<class TUnaryFunc> requires std::is_invocable_v<TUnaryFunc, T&>
void Pool<T>::ForEach(TUnaryFunc aFunc) 
{
#ifdef ASSERT_MUTEX
	// iterating prohibits allocating/freeing!
	AssertWriteLock lock(myIterationMutex);
#endif
	DEBUG_ONLY(size_t foundCount = 0;);
	for (Element& element : myElements)
	{
		if (T* value = std::get_if<1>(&element.myDataVariant))
		{
			aFunc(*value);
			DEBUG_ONLY(foundCount++;);
		}
	}
	ASSERT_STR(foundCount == mySize, "Weird PoolHeaderElem behavior, failed to find all elements!");
}

template<class T>
template<class TUnaryFunc> requires std::is_invocable_v<TUnaryFunc, const T&>
void Pool<T>::ForEach(TUnaryFunc aFunc) const 
{
#ifdef ASSERT_MUTEX
	// iterating prohibits allocating/freeing!
	AssertWriteLock lock(myIterationMutex);
#endif

	Profiler::ScopedMark mark("Pool::ForEach");

	DEBUG_ONLY(size_t foundCount = 0;);
	for (const Element& element : myElements)
	{
		if (const T* value = std::get_if<1>(&element.myDataVariant))
		{
			aFunc(*value);
			DEBUG_ONLY(foundCount++;);
		}
	}
	ASSERT_STR(foundCount == mySize, "Weird PoolHeaderElem behavior, failed to find all elements!");
}

template<class T>
template<class TUnaryFunc> requires std::is_invocable_v<TUnaryFunc, T&>
void Pool<T>::ParallelForEach(TUnaryFunc aFunc) 
{
#ifdef ASSERT_MUTEX
	// iterating prohibits allocating/freeing!
	AssertWriteLock lock(myIterationMutex);
#endif

	DEBUG_ONLY(std::atomic<size_t> foundCount = 0;);
	// Attempt to have 2 batches per thread, so that it's easier to schedule around
	const size_t batchSize = std::max(myElements.size() / std::thread::hardware_concurrency() / 2, 1ull);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, myElements.size(), batchSize),
		[&](tbb::blocked_range<size_t> aRange)
	{
		Profiler::ScopedMark mark("Pool::ForEachBatch");

		for (size_t i = aRange.begin(); i < aRange.end(); i++)
		{
			Element& element = myElements[i];
			if (T* value = std::get_if<1>(&element.myDataVariant))
			{
				aFunc(*value);
				DEBUG_ONLY(foundCount++;);
			}
		}
	});
	ASSERT_STR(foundCount == mySize, "Weird PoolHeaderElem behavior, failed to find all elements!");
}

template<class T>
template<class TUnaryFunc> requires std::is_invocable_v<TUnaryFunc, const T&>
void Pool<T>::ParallelForEach(TUnaryFunc aFunc) const 
{
#ifdef ASSERT_MUTEX
	// iterating prohibits allocating/freeing!
	AssertWriteLock lock(myIterationMutex);
#endif

	DEBUG_ONLY(std::atomic<size_t> foundCount = 0;);
	// Attempt to have 2 batches per thread, so that it's easier to schedule around
	const size_t batchSize = std::max(myElements.size() / std::thread::hardware_concurrency() / 2, 1ull);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, myElements.size(), batchSize),
		[&](tbb::blocked_range<size_t> aRange)
	{
		Profiler::ScopedMark mark("Pool::ForEachBatch");

		for (size_t i = aRange.begin(); i < aRange.end(); i++)
		{
			const Element& element = myElements[i];
			if (const T* value = std::get_if<1>(&element.myDataVariant))
			{
				aFunc(*value);
				DEBUG_ONLY(foundCount++;);
			}
		}
	});
	ASSERT_STR(foundCount == mySize, "Weird PoolHeaderElem behavior, failed to find all elements!");
}

template<class T>
void Pool<T>::Resize(size_t aSize)
{
	const size_t oldSize = myElements.size();
	myElements.resize(aSize);
	for (size_t i = oldSize; i < aSize; i++)
	{
		myElements[i].myDataVariant = i + 1;
	}
}

template<class T>
void Pool<T>::Free(size_t anIndex)
{
#ifdef ASSERT_MUTEX
	// Can't free while iterating!
	AssertReadLock lock(myIterationMutex);
#endif

	ASSERT_STR(myElements[anIndex].myDataVariant.index() == 1, "Tried to double free!");

	// Insert the free slot at the start of the free-list
	myElements[anIndex].myDataVariant = myFirstFreeSlot;
	myFirstFreeSlot = anIndex;
	mySize--;
}

template<class T>
bool Pool<T>::IsValid(size_t anIndex, GenerationType aGeneration) const
{
	return myElements[anIndex].myDataVariant.index() == 1 && myElements[anIndex].myGeneration == aGeneration;
}

// =======================
// Convenience Ptr Aliases
// =======================
template<class T>
using PoolPtr = typename Pool<T>::Ptr;

template<class T>
using PoolWeakPtr = typename Pool<T>::WeakPtr;