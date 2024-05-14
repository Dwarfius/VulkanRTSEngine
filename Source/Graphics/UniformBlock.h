#pragma once

class UniformBuffer;

class UniformBlock
{
public:
	UniformBlock(UniformBuffer& aBuffer);
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
	UniformBuffer& myBuffer;
};