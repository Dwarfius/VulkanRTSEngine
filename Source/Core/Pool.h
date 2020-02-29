#pragma once

template<class TTraits>
class Pool
{
	constexpr static uint16_t InvalidGen = 0;
public:
	template<class TElem, size_t AllocStrategy = 2, size_t InitCount = 32>
	class StaticTraits
	{
	public:
		using Element = TElem;
		// Get size, in bytes, of a single element
		size_t GetSize() const { return sizeof(Element); }
		// Get new allocation size, in elements, based on previous count
		size_t GetNextCount(size_t aOldCount) const { return aOldCount * AllocStrategy; }
		// Gets the initial number of elements
		size_t GetInitialCount() const { return InitCount; }
	};

	// RAII style handle to a pool-allocated object
	class Handle
	{
	public:
		Handle();
		~Handle();
		Handle(const Handle&) = delete;
		Handle& operator=(const Handle&) = delete;
		Handle(Handle&& aHandle);
		Handle& operator=(Handle&& aHandle);

		bool IsValid() const;
		typename TTraits::Element* Get();

	private:
		friend class Pool<TTraits>;
		Handle(Pool<TTraits>* aPool, size_t aIndex, uint16_t aGeneration);

		Pool<TTraits>* myPool; // not owned
		size_t myIndex;
		uint16_t myGeneration;
	};

	Pool(TTraits aTraits = TTraits());
	~Pool();

	// Allocates a new element and returns a Handle for it.
	// Can cause pool to resize. Handles will remain valid.
	Handle Allocate();

	// Returns whether next allocation will cause the pool to grow
	bool IsFull() const;

private:
	struct Header
	{
		Header* myNextFreeItem;
		uint16_t myGeneration;
	};

	friend class Handle;

	size_t GetIndexOfHeader(Header* aHeader) const;
	Header* GetHeader(size_t aIndex);
	const Header* GetHeader(size_t aIndex) const;
	typename TTraits::Element* GetElement(size_t aIndex);

	void Free(size_t anIndex);
	void Resize(size_t aNewCount);

	TTraits myTraits;
	
	// Data is organized in 2 halfs - headers first, then actual storage
	// For purposes of speeding up traversal of headers in search for free spots
	// as well as potentially speeding up relocations by allowing to avoid
	// full buffer copy
	char* myData;
	size_t myCount;
	Header* myFirstFreeSlot;
};

// ======================
//        HANDLE
// ======================

template<class TTraits>
Pool<TTraits>::Handle::Handle()
	: Handle(nullptr, 0, InvalidGen)
{
}

template<class TTraits>
Pool<TTraits>::Handle::Handle(Pool<TTraits>* aPool, size_t aIndex, uint16_t aGeneration)
	: myPool(aPool)
	, myIndex(aIndex)
	, myGeneration(aGeneration)
{
}

template<class TTraits>
Pool<TTraits>::Handle::~Handle()
{
	if (myPool && myGeneration != InvalidGen)
	{
		myPool->Free(myIndex);
	}
}

template<class TTraits>
Pool<TTraits>::Handle::Handle(Handle&& aHandle)
{
	*this = std::move(aHandle);
}

template<class TTraits>
typename Pool<TTraits>::Handle& Pool<TTraits>::Handle::operator=(Handle&& aHandle)
{
	myPool = aHandle.myPool;
	myIndex = aHandle.myIndex;
	myGeneration = aHandle.myGeneration;
	aHandle.myGeneration = InvalidGen;
	aHandle.myPool = nullptr;
	return *this;
}

template<class TTraits>
bool Pool<TTraits>::Handle::IsValid() const
{
	ASSERT_STR(myGeneration != InvalidGen, "Moved from handle spotted!");
	return myPool && myGeneration == myPool->GetHeader(myIndex)->myGeneration;
}

template<class TTraits>
typename TTraits::Element* Pool<TTraits>::Handle::Get()
{
	ASSERT_STR(IsValid(), "Invalid handle!");
	return myPool->GetElement(myIndex);
}

// ======================
//          POOL
// ======================

template<class TTraits>
Pool<TTraits>::Pool(TTraits aTraits /*= TTraits()*/)
	: myTraits(aTraits)
	, myCount(0)
	, myData(nullptr)
	, myFirstFreeSlot(nullptr)
{
	Resize(myTraits.GetInitialCount())
	myFirstFreeSlot = GetHeader(0);
}

template<class TTraits>
Pool<TTraits>::~Pool()
{
	delete[] myData;
}

template<class TTraits>
typename Pool<TTraits>::Handle Pool<TTraits>::Allocate()
{
	// resize if needed
	if (IsFull())
	{
		Resize(myTraits.GetNextCount(myCount));
	}

	// find first free spot
	Header* freeSpot = myFirstFreeSlot;
	const size_t currIndex = GetIndexOfHeader(freeSpot);

	// move the pointer to next free slot
	Header* secondFreeSpot = myFirstFreeSlot->myNextFreeItem;
	myFirstFreeSlot = secondFreeSpot;
	
	// finally construct the new one
	freeSpot->myNextFreeItem = nullptr;
	freeSpot->myGeneration++;
	return Handle(this, myData, freeSpot->myGeneration);
}

template<class TTraits>
bool Pool<TTraits>::IsFull() const
{
	return myFirstFreeSlot == nullptr;
}

template<class TTraits>
void Pool<TTraits>::Free(size_t anIndex)
{
	Header* header = GetHeader(anIndex);
	if (myFirstFreeSlot)
	{
		// insert the item in the free-chain towards the start
		Header* secondFreeSlot = myFirstFreeSlot->myNextFreeSlot;
		myFirstFreeSlot->myNextFreeSlot = header;
		header->myNextFreeSlot = secondFreeSlot;
	}
	else
	{
		// start a new chain
		myFirstFreeSlot = header;
	}
}

template<class TTraits>
void Pool<TTraits>::Resize(size_t aNewCount)
{
	ASSERT_STR(aNewCount, "Invalid size argument!");

	// given a new size, allocate a new space,
	// copy all existing headers, setting up the new
	// headers as part of the free chain, 
	// and discard the data itself
	size_t oldCount = myCount;
	char* oldData = myData;
	
	myCount = aNewCount;
	// Undefined Behavior warning! Currently, this implementation
	// is illegal due to UB of not creating objects in myData buffer.
	// For this to be fully legal p0593 needs to be acepted/implemented.
	const size_t newSize = myCount * (sizeof(Header) + myTraits.GetSize());
	myData = new char[newSize];

	// copy across old headers, if available
	if (oldCount)
	{
		std::memcpy(myData, oldData, oldCount * sizeof(Header));
		GetHeader(oldCount - 1)->myNextFreeSlot = GetHeader(oldCount);
	}
	// setup a free-chain within newly allocated headers
	for (size_t elemInd = oldCount; elemInd < myCount-1; elemInd++)
	{
		GetHeader(elemInd)->myNextFreeSlot = GetHeader(elemInd+1);
	}
	// last one is occupied
	GetHeader(myCount - 1)->myNextFreeSlot = nullptr;
}

template<class TTraits>
size_t Pool<TTraits>::GetIndexOfHeader(Header* aHeader) const
{
	const size_t offset = reinterpret_cast<char*>(freeSpot) - myData;
	return offset / sizeof(Header);
}

template<class TTraits>
typename Pool<TTraits>::Header* Pool<TTraits>::GetHeader(size_t aIndex)
{
	const size_t headerOffset = aIndex * sizeof(Header);
	return reinterpret_cast<Header*>(myData + headerOffset);
}

template<class TTraits>
const typename Pool<TTraits>::Header* Pool<TTraits>::GetHeader(size_t aIndex) const
{
	const size_t headerOffset = aIndex * sizeof(Header);
	return reinterpret_cast<const Header*>(myData + headerOffset);
}

template<class TTraits>
typename TTraits::Element* Pool<TTraits>::GetElement(size_t aIndex)
{
	const size_t headersEndOffset = myCount * sizeof(Header);
	const size_t elementOffset = headersEndOffset + aIndex * myTraits.GetSize();
	return reinterpret_cast<typename TTraits::Element*>(myData + elementOffset);
}