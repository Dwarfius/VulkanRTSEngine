#pragma once

class GPUBuffer;

class UniformBlock
{
public:
	UniformBlock(GPUBuffer& aBuffer);
	~UniformBlock();

	template<class T>
	void SetUniform(size_t anOffset, T&& aValue)
	{
		using TElem = std::decay_t<T>;
		TElem* slotPointer = reinterpret_cast<TElem*>(myData + anOffset);
		*slotPointer = std::forward<T>(aValue);
	}

private:
	char* myData;
	GPUBuffer& myBuffer;
};