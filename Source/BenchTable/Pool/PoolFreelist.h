#pragma once

#include <vector>

namespace BenchTable {
	// A collection object that has ability to grow like a vector, 
	// while guarantying pointer stability (via proxy type called Ptr).
	// The purpose is to help maintain cache coherence
	// Note: can be adapted as a backing storage for a pooling allocator
	template<class T>
	class PoolFreeList
	{
		using GenerationType = uint8_t;

		constexpr static uint8_t kGrowthFactor = 2;
		constexpr static uint8_t kInitialCapacity = 64;
		constexpr static int kBitsUsed = std::numeric_limits<GenerationType>::digits - 1; // we save top bit for the flag
		constexpr static GenerationType kMaxGeneration = (1ull << kBitsUsed) - 1;
	public:
		// RAII style Pointer to a pool-allocated object
		class Ptr
		{
		public:
			Ptr() = default;
			Ptr(const Ptr&) = delete;
			Ptr& operator=(const Ptr&) = delete;

			Ptr(Ptr&& aPtr) noexcept;
			Ptr& operator=(Ptr&& aPtr) noexcept;

			~Ptr();

			bool IsValid() const;
			T* Get();

		private:
			friend class PoolFreeList<T>;
			Ptr(PoolFreeList<T>* aPool, size_t anIndex, GenerationType aGeneration) noexcept;

			PoolFreeList<T>* myPool = nullptr; // not owned
			size_t myIndex = 0;
			GenerationType myGeneration = 0;
		};

		// Allocates a new element and returns a Ptr for it.
		// Can cause pool to resize. Ptrs will remain valid.
		template<class... TArgs>
		Ptr Allocate(TArgs&&... aConstrArgs);

		template<class TUnaryFunc,
			class = std::enable_if_t<std::is_invocable_v<TUnaryFunc, T&>>>
			void ForEach(TUnaryFunc aFunc);

		template<class TUnaryFunc,
			class = std::enable_if_t<std::is_invocable_v<TUnaryFunc, const T&>>>
			void ForEach(TUnaryFunc aFunc) const;

		size_t GetCapacity() const { return myElements.capacity(); }
		size_t GetSize() const { return mySize; }

		// Exposed for benching
		// Don't use AllocateAt if you're going to be adding more 
		// - it doesn't handle free slots!
		template<class... TArgs>
		Ptr AllocateAt(size_t anIndex, TArgs&&... aConstrArgs);
		void* GetInternalBuffer() { return myElements.data(); }
		void Resize(size_t aNewCount);

	private:
		friend class Ptr;

		struct Element
		{
			union
			{
				size_t myNextFreeSlot;
				T myValue;
			};
			GenerationType myGeneration : kBitsUsed;
			GenerationType myIsActive : 1;

			Element() noexcept
				: myNextFreeSlot(0)
				, myGeneration(0)
				, myIsActive(false)
			{
			}
		};

		void Free(size_t anIndex);
		bool IsValid(size_t anIndex, GenerationType aGeneration) const;

		std::vector<Element> myElements;
		size_t myFirstFreeSlot = 0;
		size_t mySize = 0;
	};

	// ======================
	//        Ptr
	// ======================

	template<class T>
	PoolFreeList<T>::Ptr::Ptr(PoolFreeList<T>* aPool, size_t anIndex, GenerationType aGeneration) noexcept
		: myPool(aPool)
		, myIndex(anIndex)
		, myGeneration(aGeneration)
	{
	}

	template<class T>
	PoolFreeList<T>::Ptr::~Ptr()
	{
		if (IsValid())
		{
			myPool->Free(myIndex);
		}
	}

	template<class T>
	PoolFreeList<T>::Ptr::Ptr(Ptr&& aPtr) noexcept
	{
		*this = std::move(aPtr);
	}

	template<class T>
	typename PoolFreeList<T>::Ptr& PoolFreeList<T>::Ptr::operator=(Ptr&& aPtr) noexcept
	{
		if (IsValid())
		{
			myPool->Free(myIndex);
		}
		myPool = aPtr.myPool;
		myIndex = aPtr.myIndex;
		myGeneration = aPtr.myGeneration;
		aPtr.myPool = nullptr;
		aPtr.myIndex = 0;
		aPtr.myGeneration = 0;
		return *this;
	}

	template<class T>
	bool PoolFreeList<T>::Ptr::IsValid() const
	{
		return myPool && myPool->IsValid(myIndex, myGeneration);
	}

	template<class T>
	T* PoolFreeList<T>::Ptr::Get()
	{
		ASSERT_STR(IsValid(), "Invalid Ptr!");
		return &myPool->myElements[myIndex].myValue;
	}

	// ======================
	//         Pool
	// ======================

	template<class T>
	template<class... TArgs>
	typename PoolFreeList<T>::Ptr PoolFreeList<T>::Allocate(TArgs&&... aConstrArgs)
	{
		if (mySize == myElements.capacity())
		{
			const size_t newSize = myElements.capacity() ? myElements.capacity() * kGrowthFactor : kInitialCapacity;
			Resize(newSize);
		}

		// find first free spot
		const size_t currIndex = myFirstFreeSlot;
		Element& currFreeSpot = myElements[currIndex];
		ASSERT_STR(!currFreeSpot.myIsActive, "Weird pool state detected!");

		// move the pointer to next free slot
		myFirstFreeSlot = currFreeSpot.myNextFreeSlot;

		// finally construct the new one
		new(static_cast<void*>(&currFreeSpot.myValue)) T(std::forward<TArgs...>(aConstrArgs)...);
		currFreeSpot.myIsActive = true;
		currFreeSpot.myGeneration = currFreeSpot.myGeneration == kMaxGeneration ? 0 : currFreeSpot.myGeneration++;
		mySize++;
		
		return Ptr(this, currIndex, currFreeSpot.myGeneration);
	}

	template<class T>
	template<class TUnaryFunc, class /* = std::enable_if_t<std::is_invocable_v<TUnaryFunc, T&>>*/>
	void PoolFreeList<T>::ForEach(TUnaryFunc aFunc)
	{
		DEBUG_ONLY(size_t foundCount = 0;);
		for (Element& element : myElements)
		{
			if (element.myIsActive)
			{
				aFunc(element.myValue);
				DEBUG_ONLY(foundCount++;);
			}
		}
		ASSERT_STR(foundCount == mySize, "Weird PoolHeaderElem behavior, failed to find all elements!");
	}

	template<class T>
	template<class TUnaryFunc, class /* = std::enable_if_t<std::is_invocable_v<TUnaryFunc, const T&>>*/>
	void PoolFreeList<T>::ForEach(TUnaryFunc aFunc) const
	{
		DEBUG_ONLY(size_t foundCount = 0);
		for (const Element& element : myElements)
		{
			if (element.myIsActive)
			{
				aFunc(element.myValue);
				DEBUG_ONLY(foundCount++);
			}
		}
		ASSERT_STR(foundCount == mySize, "Weird PoolHeaderElem behavior, failed to find all elements!");
	}

	template<class T>
	template<class... TArgs>
	typename PoolFreeList<T>::Ptr PoolFreeList<T>::AllocateAt(size_t anIndex, TArgs&&... aConstrArgs)
	{
		ASSERT(anIndex < myElements.size());

		Element& element = myElements[anIndex];
		if (element.myIsActive)
		{
			return Ptr();
		}

		new (static_cast<void*>(&element.myValue)) T(std::forward<TArgs...>(aConstrArgs...));
		element.myIsActive = true;
		element.myGeneration = element.myGeneration == kMaxGeneration ? 0 : ++element.myGeneration;
		mySize++;
		return Ptr(this, anIndex, element.myGeneration);
	}

	template<class T>
	void PoolFreeList<T>::Resize(size_t aSize)
	{
		const size_t oldSize = myElements.size();
		myElements.resize(aSize);
		for (size_t i = oldSize; i < aSize; i++)
		{
			myElements[i].myNextFreeSlot = i + 1;
		}
	}

	template<class T>
	void PoolFreeList<T>::Free(size_t anIndex)
	{
		ASSERT_STR(myElements[anIndex].myIsActive, "Tried to double free!");

		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			&(myElements[anIndex].myElement)->~T();
		}

		// Insert the free slot at the start of the free-list
		myElements[anIndex].myNextFreeSlot = myFirstFreeSlot;
		myElements[anIndex].myIsActive = false;
		myFirstFreeSlot = anIndex;
		mySize--;
	}

	template<class T>
	bool PoolFreeList<T>::IsValid(size_t anIndex, GenerationType aGeneration) const
	{
		return myElements[anIndex].myIsActive && myElements[anIndex].myGeneration == aGeneration;
	}
};