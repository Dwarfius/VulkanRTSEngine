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
	void Square(uint32_t aTopLeftX, uint32_t aTopLeftY, uint32_t aSize, TDistr& aDistr, TElem* aData);
	void Diamond(uint32_t aCenterX, uint32_t aCenterY, uint32_t aHalfSize, TDistr& aDistr, TElem* aData);

	std::mt19937 myEngine;
	const uint32_t mySize;
	const TElem myMin;
	const TElem myMax;
};

template<class TElem>
void DiamondSquareAlgo<TElem>::Generate(TElem* aData)
{
	// using min-max range for seeding initially
	TDistr distribution(myMin, myMax);

	// seed the first 4 corners
	aData[GetCoord(0, 0)] = distribution(myEngine);
	aData[GetCoord(mySize-1, 0)] = distribution(myEngine);
	aData[GetCoord(0, mySize-1)] = distribution(myEngine);
	aData[GetCoord(mySize - 1, mySize - 1)] = distribution(myEngine);

	// start iterating
	// Note: this implementation will apply Diamond to some points twice,
	// but it's okay, because first pass will have partial data,
	// while the second pass will overwrite with full data
	// Drawback is that it's not as efficient
	TElem distrRange = (myMax - myMin);
	for (uint32_t currSize = mySize - 1; currSize > 1; currSize >>= 1)
	{
		// we use relative range, since we're adding 
		// to heights that were seeded initially via Min-Max
		const TElem currMin = -distrRange;
		const TElem currMax = +distrRange;
		distribution = TDistr(currMin, currMax);
		distrRange /= 2;

		const uint32_t halfSize = currSize >> 1;
		for (uint32_t y = 0; y < mySize; y += currSize)
		{
			for (uint32_t x = 0; x < mySize; x += currSize)
			{
				// top left corner
				if (x < mySize - 1 && y < mySize - 1)
				{
					Square(x, y, currSize, distribution, aData);
				}
				// top
				if (x + halfSize < mySize)
				{
					Diamond(x + halfSize, y, halfSize, distribution, aData);
				}
				// left
				if (y + halfSize < mySize)
				{
					Diamond(x, y + halfSize, halfSize, distribution, aData);
				}
				// bottom
				if (x + halfSize < mySize && y + currSize < mySize)
				{
					Diamond(x + halfSize, y + currSize, halfSize, distribution, aData);
				}
				// right
				if (x + currSize < mySize && y + halfSize < mySize)
				{
					Diamond(x + currSize, y + halfSize, halfSize, distribution, aData);
				}
			}
		}
	}
}

template<class TElem>
void DiamondSquareAlgo<TElem>::Square(uint32_t aTopLeftX, uint32_t aTopLeftY, uint32_t aSize, TDistr& aDistr, TElem* aData)
{
	TElem sum = aData[GetCoord(aTopLeftX, aTopLeftY)];
	sum += aData[GetCoord(aTopLeftX + aSize, aTopLeftY)];
	sum += aData[GetCoord(aTopLeftX, aTopLeftY + aSize)];
	sum += aData[GetCoord(aTopLeftX + aSize, aTopLeftY + aSize)];
	const uint32_t halfSize = aSize >> 1;
	const uint32_t center = GetCoord(aTopLeftX + halfSize, aTopLeftY + halfSize);
	ASSERT(center < mySize * mySize);
	TElem newVal = sum / 4 + aDistr(myEngine);
	aData[center] = glm::clamp(newVal, myMin, myMax);
}

template<class TElem>
void DiamondSquareAlgo<TElem>::Diamond(uint32_t aCenterX, uint32_t aCenterY, uint32_t aHalfSize, TDistr& aDistr, TElem* aData)
{
	ASSERT(aCenterX < mySize && aCenterY < mySize);
	uint8_t count = 0;
	TElem sum = 0;
	if (aCenterX >= aHalfSize)
	{
		sum += aData[GetCoord(aCenterX - aHalfSize, aCenterY)];
		count++;
	}
	if (aCenterY >= aHalfSize)
	{
		sum += aData[GetCoord(aCenterX, aCenterY - aHalfSize)];
		count++;
	}
	if (aCenterX < mySize - aHalfSize)
	{
		sum += aData[GetCoord(aCenterX + aHalfSize, aCenterY)];
		count++;
	}
	if (aCenterY < mySize - aHalfSize)
	{
		sum += aData[GetCoord(aCenterX, aCenterY + aHalfSize)];
		count++;
	}
	ASSERT(count >= 3);
	const uint32_t center = GetCoord(aCenterX, aCenterY);
	TElem newVal = sum / count + aDistr(myEngine);
	aData[center] = glm::clamp(newVal, myMin, myMax);
}