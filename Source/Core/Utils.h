#pragma once

namespace Utils
{
	template<class T>
	uint8_t CountSetBits(T aVal);

	bool IsNan(glm::vec3 aVec);
	bool IsNan(glm::vec4 aVec);
}

template<class T>
uint8_t Utils::CountSetBits(T aVal)
{
	static_assert(std::is_integral_v<T>, "T mush be integral!");
	uint8_t bitCount = 0;
	while (aVal)
	{
		bitCount += aVal & 1;
		aVal >>= 1;
	}
	return bitCount;
}