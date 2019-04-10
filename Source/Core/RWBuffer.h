#pragma once

#include "Debug/Assert.h"

#include <array>

// A utility buffer which tracks Read and Write positions individually  
// and asserts if read-write collision happens. Read and Write heads
// can be pointing towards the same element, but calling either 
// Read/WriteCurrent() will trigger an assert
template<typename T, size_t Size>
class RWBuffer
{
	using InternalBuffer = array<T, Size>;
	using InternalIter = typename InternalBuffer::iterator;
public:
	// construct a buffer with first element default initialized
	RWBuffer();
	// construct a buffer with first element copied in
	RWBuffer(const T& aFirstElem);
	// construct a buffer with first element moved in
	RWBuffer(T&& aFirstElem);

	constexpr size_t GetSize() const { return Size; }

	T& GetWrite();
	const T& GetRead() const;

	void AdvanceWrite();
	void AdvanceRead();
	// shortcut for advancing both the reader and the writer
	void Advance() { AdvanceWrite(); AdvanceRead(); }
	
	// range-for utility support
	InternalIter begin() { return myBuffers.begin(); }
	InternalIter end() { return myBuffers.end(); }

private:
	InternalBuffer myBuffers;
	size_t myReadBuffer;
	size_t myWriteBuffer;
	// false if write is ahead of read, true if write just wrapped and read didn't yet
	bool mySwapped;
};

// ============================================================================

template<typename T, size_t Size>
RWBuffer<T, Size>::RWBuffer()
	: myReadBuffer(0)
	, myWriteBuffer(1)
	, mySwapped(false)
{
	myBuffers[0] = T();
}

template<typename T, size_t Size>
RWBuffer<T, Size>::RWBuffer(const T& aFirstElem)
	: myReadBuffer(0)
	, myWriteBuffer(1)
	, mySwapped(false)
{
	myBuffers[0] = aFirstElem;
}

template<typename T, size_t Size>
RWBuffer<T, Size>::RWBuffer(T&& aFirstElem)
	: myReadBuffer(0)
	, myWriteBuffer(1)
	, mySwapped(false)
{
	myBuffers[0] = move(aFirstElem); // named rvalue ref is an lvalue, force it back to rvalue
}

template<typename T, size_t Size>
T& RWBuffer<T, Size>::GetWrite()
{
	ASSERT_STR(myWriteBuffer != myReadBuffer, "Tried to write to a read buffer!");
	return myBuffers[myWriteBuffer];
}

template<typename T, size_t Size>
const T& RWBuffer<T, Size>::GetRead() const
{
	ASSERT_STR(myWriteBuffer != myReadBuffer, "Tried to read from a write buffer!");
	return myBuffers[myReadBuffer];
}

template<typename T, size_t Size>
void RWBuffer<T, Size>::AdvanceWrite()
{
	// write has to always not fall behind of read
	myWriteBuffer = ++myWriteBuffer % Size;
	if (!myWriteBuffer)
	{
		mySwapped = true; // write has wrapped, use swapped order checks
	}
	DEBUG_ONLY(
		bool hasNotOverrun = mySwapped ?
			myWriteBuffer <= myReadBuffer :
			myWriteBuffer >= myReadBuffer;
	);
	ASSERT_STR(hasNotOverrun, "Write head has stepped over Read head!");
}

template<typename T, size_t Size>
void RWBuffer<T, Size>::AdvanceRead()
{
	// read has to always not run ahead of write
	myReadBuffer = ++myReadBuffer % Size;
	if (!myReadBuffer)
	{
		mySwapped = false; // read caught up, back to normal order
	}
	DEBUG_ONLY(
		bool hasNotOverrun = mySwapped ?
			myWriteBuffer <= myReadBuffer :
			myWriteBuffer >= myReadBuffer;
	);
	ASSERT_STR(hasNotOverrun, "Read head has stepped over Write head!");
}

// ============================================================================

// A RWBuffer-utility specialization which exposes a simplified interface.
// Does not perform any assertions
template<typename T>
class RWBuffer<T, 2>
{
	using InternalBuffer = array<T, 2>;
	using InternalIter = typename InternalBuffer::iterator;
public:
	// construct a buffer with first element default initialized
	RWBuffer();
	// construct a buffer with first element copied in
	RWBuffer(const T& aFirstElem);
	// construct a buffer with first element moved in
	RWBuffer(T&& aFirstElem);

	constexpr size_t GetSize() const { return 2; }

	T& GetWrite() { return myBuffers[myWriteBuffer ? 1 : 0]; }
	const T& GetRead() const { return myBuffers[myWriteBuffer ? 0 : 1]; }

	void Swap() { myWriteBuffer ^= true; }

private:
	InternalBuffer myBuffers;
	bool myWriteBuffer; // if true, points to 2nd one
};

// ============================================================================

template<typename T>
RWBuffer<T, 2>::RWBuffer()
	: myWriteBuffer(true)
{
	myBuffers[0] = T();
}

template<typename T>
RWBuffer<T, 2>::RWBuffer(const T& aFirstElem)
	: myWriteBuffer(true)
{
	myBuffers[0] = aFirstElem;
}

template<typename T>
RWBuffer<T, 2>::RWBuffer(T&& aFirstElem)
	: myWriteBuffer(true)
{
	myBuffers[0] = move(aFirstElem); // named rvalue ref is an lvalue, force it back to rvalue
}