#pragma once

namespace BenchTable {
	// A collection object that has ability to grow like a vector, 
	// while guarantying pointer stability (via proxy type called Ptr).
	// The purpose is to help maintain cache coherence
	// Note: can be adapted as a backing storage for a PoolHeaderEleming allocator
	template<class T>
	class PoolHeaderElem
	{
		static_assert(sizeof(T) >= 0, "T must be a complete type!");
		static_assert(alignof(T) <= sizeof(T), "Unusual alignment requirements not supported");

		// if you want to support more items, pick a larger index type
		using IndexType = size_t;
		using GenerationType = uint8_t;

		constexpr static uint8_t kGrowthFactor = 2;
		constexpr static uint8_t kInitialCapacity = 64;
		constexpr static int kBitsUsed = std::numeric_limits<GenerationType>::digits - 1; // we save top bit for the flag
		constexpr static GenerationType kMaxGeneration = (1ull << kBitsUsed) - 1;
	public:
		// RAII style Pointer to a PoolHeaderElem-allocated object
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
			friend class PoolHeaderElem<T>;
			Ptr(PoolHeaderElem<T>* aPoolHeaderElem, IndexType anIndex, GenerationType aGeneration) noexcept;

			PoolHeaderElem<T>* myPoolHeaderElem = nullptr; // not owned
			IndexType myIndex = 0;
			GenerationType myGeneration = 0;
		};

		~PoolHeaderElem();

		// Allocates a new element and returns a Ptr for it.
		// Can cause PoolHeaderElem to resize. Ptrs will remain valid.
		template<class... TArgs>
		Ptr Allocate(TArgs&&... aConstrArgs);

		template<class TUnaryFunc,
			class = std::enable_if_t<std::is_invocable_v<TUnaryFunc, T&>>>
			void ForEach(TUnaryFunc aFunc);

		template<class TUnaryFunc,
			class = std::enable_if_t<std::is_invocable_v<TUnaryFunc, const T&>>>
			void ForEach(TUnaryFunc aFunc) const;

		IndexType GetCapacity() const { return myCapacity; }
		IndexType GetSize() const { return mySize; }

		// Exposed for benching
		template<class... TArgs>
		Ptr AllocateAt(size_t anIndex, TArgs&&... aConstrArgs);
		char* GetInternalBuffer() { return myData; }
		void Resize(IndexType aNewCount);

	private:
		friend class Ptr;

		struct Header
		{
			IndexType myNextFreeIndex;
			GenerationType myGeneration : kBitsUsed;
			GenerationType myIsInUse : 1;
		};

		Header* GetHeader(IndexType anIndex);
		const Header* GetHeader(IndexType anIndex) const;
		T* GetElement(IndexType anIndex);
		const T* GetElement(IndexType anIndex) const;
		size_t GetElementOffset(IndexType anIndex) const;

		void Free(IndexType anIndex);
		bool IsValid(size_t anIndex, GenerationType aGeneration) const;

		// Data is organized in 2 halfs - headers first, then actual storage
		// as in: | Header0 | Header1 | Header2 | ... | Elem0 | Elem1 | Elem2 | ...
		// For purposes of speeding up traversal of headers in search for free spots
		// as well as potentially speeding up relocations by allowing to avoid
		// full buffer copy (need to copy all Headers, but not all Elems)
		char* myData = nullptr;
		IndexType myFirstFreeSlot = 0;
		IndexType myCapacity = 0;
		IndexType mySize = 0;
	};

	// ======================
	//        Ptr
	// ======================

	template<class T>
	PoolHeaderElem<T>::Ptr::Ptr(PoolHeaderElem<T>* aPoolHeaderElem, IndexType anIndex, GenerationType aGeneration) noexcept
		: myPoolHeaderElem(aPoolHeaderElem)
		, myIndex(anIndex)
		, myGeneration(aGeneration)
	{
	}

	template<class T>
	PoolHeaderElem<T>::Ptr::~Ptr()
	{
		if (IsValid())
		{
			myPoolHeaderElem->Free(myIndex);
		}
	}

	template<class T>
	PoolHeaderElem<T>::Ptr::Ptr(Ptr&& aPtr) noexcept
	{
		*this = std::move(aPtr);
	}

	template<class T>
	typename PoolHeaderElem<T>::Ptr& PoolHeaderElem<T>::Ptr::operator=(Ptr&& aPtr) noexcept
	{
		if (IsValid())
		{
			myPoolHeaderElem->Free(myIndex);
		}
		myPoolHeaderElem = aPtr.myPoolHeaderElem;
		myIndex = aPtr.myIndex;
		myGeneration = aPtr.myGeneration;
		aPtr.myPoolHeaderElem = nullptr;
		aPtr.myIndex = 0;
		aPtr.myGeneration = 0;
		return *this;
	}

	template<class T>
	bool PoolHeaderElem<T>::Ptr::IsValid() const
	{
		return myPoolHeaderElem && myPoolHeaderElem->IsValid(myIndex, myGeneration);
	}

	template<class T>
	T* PoolHeaderElem<T>::Ptr::Get()
	{
		ASSERT_STR(IsValid(), "Invalid Ptr!");
		return &myPoolHeaderElem->GetElement(myIndex);
	}

	// ======================
	//     PoolHeaderElem
	// ======================

	template<class T>
	PoolHeaderElem<T>::~PoolHeaderElem()
	{
		delete[] myData;
	}

	template<class T>
	template<class... TArgs>
	typename PoolHeaderElem<T>::Ptr PoolHeaderElem<T>::Allocate(TArgs&&... aConstrArgs)
	{
		// resize if needed
		if (mySize == myCapacity)
		{
			const IndexType newSize = myCapacity ? myCapacity * kGrowthFactor : kInitialCapacity;
			Resize(newSize);
		}

		// find first free spot
		const IndexType currIndex = myFirstFreeSlot;
		Header& currFreeSpot = *GetHeader(currIndex);
		ASSERT_STR(!currFreeSpot.myIsInUse, "Weird PoolHeaderElem state detected!");

		// move the pointer to next free slot
		myFirstFreeSlot = currFreeSpot.myNextFreeIndex;

		// finally construct the new one
		new(static_cast<void*>(GetElement(currIndex))) T(std::forward<TArgs...>(aConstrArgs)...);
		currFreeSpot.myIsInUse = true;
		currFreeSpot.myGeneration = currFreeSpot.myGeneration == kMaxGeneration ? 0 : currFreeSpot.myGeneration++;
		mySize++;
		return Ptr(this, currIndex, currFreeSpot.myGeneration);
	}

	template<class T>
	template<class TUnaryFunc, class /* = std::enable_if_t<std::is_invocable_v<TUnaryFunc, T&>>*/>
	void PoolHeaderElem<T>::ForEach(TUnaryFunc aFunc)
	{
		// I think this is not a cache friendly pattern, need to come back and improve it
		// and benchmark it. current ideas are:
		//  * swap halves, have small Headers towards the end
		//  * wrap T in Proxy { T t; bool:1 isInUse }
		//  * separate Headers into a 2 separate buffers, store inUse flag in a bitmask
		DEBUG_ONLY(IndexType foundCount = 0;);
		for (IndexType ind = 0;
			ind < myCapacity
#ifdef ENABLE_ASSERTS
			&& foundCount < mySize
#endif
			; ind++)
		{
			// TODO: [[likely]]
			if (GetHeader(ind)->myIsInUse)
			{
				aFunc(*GetElement(ind));
				DEBUG_ONLY(foundCount++;);
			}
		}
		ASSERT_STR(foundCount == mySize, "Weird PoolHeaderElem behavior, failed to find all elements!");
	}

	template<class T>
	template<class TUnaryFunc, class /* = std::enable_if_t<std::is_invocable_v<TUnaryFunc, const T&>>*/>
	void PoolHeaderElem<T>::ForEach(TUnaryFunc aFunc) const
	{
		DEBUG_ONLY(IndexType foundCount = 0;);
		for (IndexType ind = 0;
			ind < myCapacity 
#ifdef ENABLE_ASSERTS
				&& foundCount < mySize
#endif
			; ind++)
		{
			// TODO: [[likely]]
			if (GetHeader(ind)->myIsInUse)
			{
				aFunc(*GetElement(ind));
				DEBUG_ONLY(foundCount++;);
			}
		}
		ASSERT_STR(foundCount == mySize, "Weird PoolHeaderElem behavior, failed to find all elements!");
	}

	template<class T>
	void PoolHeaderElem<T>::Free(IndexType anIndex)
	{
		ASSERT_STR(mySize > 0, "Weird state of PoolHeaderElem encountered!");
		ASSERT_STR(anIndex < myCapacity, "Invalid index spotted!");

		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			GetElement(index)->~T();
		}

		// Insert the free slot at the start of the free-list
		Header* header = GetHeader(anIndex);
		header->myNextFreeIndex = myFirstFreeSlot;
		header->myIsInUse = false;
		myFirstFreeSlot = anIndex;

		mySize--;
	}

	template<class T>
	template<class... TArgs>
	typename PoolHeaderElem<T>::Ptr PoolHeaderElem<T>::AllocateAt(size_t anIndex, TArgs&&... aConstrArgs)
	{
		ASSERT(anIndex < myCapacity);

		Header* header = GetHeader(anIndex);

		if (header->myIsInUse)
		{
			return Ptr();
		}

		new(static_cast<void*>(GetElement(anIndex))) T(std::forward<TArgs...>(aConstrArgs)...);
		header->myIsInUse = true;
		header->myGeneration = header->myGeneration == kMaxGeneration ? 0 : ++header->myGeneration;
		mySize++;
		return Ptr(this, anIndex, header->myGeneration);
	}

	template<class T>
	void PoolHeaderElem<T>::Resize(IndexType aNewCount)
	{
		ASSERT_STR(aNewCount, "Invalid size argument!");

		// given a new size, allocate a new space,
		// copy all existing headers, setting up the new
		// headers as part of the free chain

		const size_t newSize = aNewCount * (sizeof(Header) + sizeof(T));
		char* newBuffer = new char[newSize];
		if (mySize)
		{
			// Copy old headers, so that we can preserve the "IsInUse" flag
			static_assert(std::is_trivial_v<Header>, "not a Implicit-lifetime type, object can't be created via memcpy!");
			const size_t fullHeaderSize = mySize * sizeof(Header);
			std::memcpy(newBuffer, myData, fullHeaderSize);

			const size_t newFullHeaderSize = aNewCount * sizeof(Header);
			// is it an implicit-lifetime type?
			// https://en.cppreference.com/w/cpp/language/lifetime#Implicit-lifetime_types
			if constexpr (std::is_trivial_v<T> && std::is_trivially_destructible_v<T>)
			{
				// Since it's a implicit-lifetime type, memcpy will implecitly 
				// create objects in destination
				// https://en.cppreference.com/w/cpp/language/object#Object_creation
				std::memcpy(newBuffer + newFullHeaderSize,
					myData + fullHeaderSize,
					mySize * sizeof(T)
				);
			}
			else
			{
				// otherwise we have to do it via move construction
				// to explicitly trigger object creation, and avoid UB when
				// some accesses T's property in newBuffer
				T* dest = reinterpret_cast<T*>(newBuffer + newFullHeaderSize);
				std::uninitialized_move_n(GetElement(0), mySize, dest);
			}
		}

		delete[] myData;
		myCapacity = aNewCount;
		myData = newBuffer;

		// setup a free-chain within newly allocated headers
		myFirstFreeSlot = mySize;
		for (IndexType elemInd = mySize; elemInd < myCapacity; elemInd++)
		{
			Header* newHeader = GetHeader(elemInd);
			new (static_cast<void*>(newHeader)) Header;
			newHeader->myNextFreeIndex = elemInd + 1;
			newHeader->myGeneration = 0;
			newHeader->myIsInUse = false;
		}
	}

	template<class T>
	typename PoolHeaderElem<T>::Header* PoolHeaderElem<T>::GetHeader(IndexType anIndex)
	{
		ASSERT_STR(anIndex < myCapacity, "Out of bounds!");
		const size_t headerOffset = anIndex * sizeof(Header);
		return reinterpret_cast<Header*>(myData + headerOffset);
	}

	template<class T>
	const typename PoolHeaderElem<T>::Header* PoolHeaderElem<T>::GetHeader(IndexType anIndex) const
	{
		ASSERT_STR(anIndex < myCapacity, "Out of bounds!");
		const size_t headerOffset = anIndex * sizeof(Header);
		return reinterpret_cast<const Header*>(myData + headerOffset);
	}

	template<class T>
	T* PoolHeaderElem<T>::GetElement(IndexType anIndex)
	{
		ASSERT_STR(anIndex < myCapacity, "Out of bounds!");
		return reinterpret_cast<T*>(myData + GetElementOffset(anIndex));
	}

	template<class T>
	const T* PoolHeaderElem<T>::GetElement(IndexType anIndex) const
	{
		ASSERT_STR(anIndex < myCapacity, "Out of bounds!");
		return reinterpret_cast<const T*>(myData + GetElementOffset(anIndex));
	}

	template<class T>
	size_t PoolHeaderElem<T>::GetElementOffset(IndexType anIndex) const
	{
		const size_t headersEndOffset = myCapacity * sizeof(Header);
		const size_t elementOffset = headersEndOffset + anIndex * sizeof(T);
		return elementOffset;
	}

	template<class T>
	bool PoolHeaderElem<T>::IsValid(size_t anIndex, GenerationType aGeneration) const
	{
		const Header* header = GetHeader(anIndex);
		return header->myIsInUse && header->myGeneration == aGeneration;
	}
};