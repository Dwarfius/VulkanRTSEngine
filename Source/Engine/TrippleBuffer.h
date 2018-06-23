#pragma once

// Declaration
template<typename T>
class TrippleBuffer
{
public:
	typedef array<T, 3> InternalBuffer;
	TrippleBuffer();

	void Advance();
	T& GetCurrent() { return myBuffers[myCurrBuffer]; }

	// returns the whole internal array
	InternalBuffer& GetInternalBuffer() { return myBuffers; }

private:
	InternalBuffer myBuffers;
	int myCurrBuffer;
};

// Definition
template<typename T>
TrippleBuffer<T>::TrippleBuffer()
	: myCurrBuffer(0)
{
}

template<typename T>
void TrippleBuffer<T>::Advance()
{
	myCurrBuffer++;
	if (myCurrBuffer == 3)
		myCurrBuffer = 0;
}