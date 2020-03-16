#pragma once

#include "../Utils.h"

template<class TElem>
class DiamondSquareAlgo
{
public:
	DiamondSquareAlgo(uint32_t aSeed, uint32_t aSize, TElem aMin, TElem aMax)
		: myEngine(aSeed)
		, mySize(aSize)
		, myMin(aMin)
		, myMax(aMax)
	{
		const uint8_t bitCount = Utils::CountSetBits(aSize);
		ASSERT_STR(bitCount == 2 && (mySize & 1), "Size must be 2N+1!");
	}

	void Generate(TElem* aData);

private:
	using TDistr = std::conditional_t<
		std::is_floating_point_v<TElem>,
		std::uniform_real_distribution<TElem>, 
		std::uniform_int_distribution<TElem>
	>;

	uint32_t GetCoord(uint32_t aX, uint32_t aY) const { return aY * mySize + aX; }
	void Square(uint32_t aTopLeftX, uint32_t aTopLeftY, uint32_t aSize, const TDistr& aDistr, TElem* aData);
	void Diamond(uint32_t aCenterX, uint32_t aCenterY, uint32_t aSize, const TDistr& aDistr, TElem* aData);

	std::mt19937 myEngine;
	const uint32_t mySize;
	const TElem myMin;
	const TElem myMax;
};

template<class TElem>
void DiamondSquareAlgo<TElem>::Generate(TElem* aData)
{
	TDistr distribution(myMin, myMax);

	// seed the first 4 corners
	aData[GetCoord(0, 0)] = distribution(myEngine);
	aData[GetCoord(mySize-1, 0)] = distribution(myEngine);
	aData[GetCoord(0, mySize-1)] = distribution(myEngine);
	aData[GetCoord(mySize - 1, mySize - 1)] = distribution(myEngine);

	// start iterating
	// Note: this implementation will apply Diamond to some points twice,
	// but it's okay, because first pass will have partial data,
	// while the second pass over the same point will have full data
	// Drawback is that it's not as efficient
	for (uint32_t currSize = mySize-1; currSize > 2; currSize >>= 1)
	{
		const uint32_t halfSize = currSize >> 1;
		for (uint32_t y = 0; y < mySize - 1; y += currSize)
		{
			for (uint32_t x = 0; x < mySize - 1; x += currSize)
			{
				Square(x, y, currSize, distribution, aData);
				Diamond(x + halfSize, y, currSize, distribution, aData);
				Diamond(x, y + halfSize, currSize, distribution, aData);
				Diamond(x + halfSize, y + currSize, currSize, distribution, aData);
				Diamond(x + currSize, y + halfSize, currSize, distribution, aData);
			}
		}
	}
}

template<class TElem>
void DiamondSquareAlgo<TElem>::Square(uint32_t aTopLeftX, uint32_t aTopLeftY, uint32_t aSize, const TDistr& aDistr, TElem* aData)
{
	TElem sum = aData[GetCoord(aTopLeftX, aTopLeftY)];
	sum += aData[GetCoord(aTopLeftX + aSize - 1, aTopLeftY)];
	sum += aData[GetCoord(aTopLeftX, aTopLeftY + aSize - 1)];
	sum += aData[GetCoord(aTopLeftX + aSize - 1, aTopLeftY + aSize - 1)];
	const uint32_t halfSize = aSize >> 1;
	const uint32_t center = GetCoord(aTopLeftX + halfSize, aTopLeftY + halfSize);
	ASSERT(center < mySize * mySize);
	aData[center] = sum / 4 + aDistr(myEngine);
}

template<class TElem>
void DiamondSquareAlgo<TElem>::Diamond(uint32_t aCenterX, uint32_t aCenterY, uint32_t aSize, const TDistr& aDistr, TElem* aData)
{
	const uint32_t halfSize = aSize >> 1;
	uint8_t count = 0;
	TElem sum = 0;
	if (aCenterX >= halfSize)
	{
		sum += aData[GetCoord(aCenterX - halfSize, aCenterY)];
		count++;
	}
	if (aCenterY >= halfSize)
	{
		sum += aData[GetCoord(aCenterX, aCenterY - halfSize)];
		count++;
	}
	if (aCenterX < mySize - halfSize)
	{
		sum += aData[GetCoord(aCenterX + halfSize, aCenterY)];
		count++;
	}
	if (aCenterY < mySize - halfSize)
	{
		sum += aData[GetCoord(aCenterX, aCenterY + halfSize)];
		count++;
	}
	ASSERT(count >= 3);
	const uint32_t center = GetCoord(aCenterX, aCenterY);
	ASSERT(center < mySize * mySize);
	aData[center] = sum / count + aDistr(myEngine);
}