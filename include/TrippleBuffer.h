#pragma once

// Declaration
template<typename T>
class TrippleBuffer
{
public:
	TrippleBuffer(size_t preAllocSize = 4000);

	void Add(const T& newElem);
	void Advance();
	const vector<T>& GetBuffer() const;

private:
	vector<T> buffers[3];
	int currBuffer;
};

// Definition
template<typename T>
TrippleBuffer<T>::TrippleBuffer(size_t preAllocSize /* = 4000 */)
	: currBuffer(0)
{
	for (int i = 0; i < 3; i++)
	{
		buffers[i] = vector<T>();
		buffers[i].reserve(preAllocSize);
	}
}

template<typename T>
void TrippleBuffer<T>::Add(const T& newElem)
{
	buffers[currBuffer].push_back(newElem);
}

template<typename T>
void TrippleBuffer<T>::Advance()
{
	currBuffer++;
	if (currBuffer == 3)
		currBuffer = 0;
	buffers[currBuffer].clear();
}

template<typename T>
const vector<T>& TrippleBuffer<T>::GetBuffer() const
{
	return buffers[currBuffer];
}