#include "Precomp.h"
#include "WaterSim.h"

void WaterSim::SeedState(std::span<float> aData, int aSideSize)
{
	myGrid.assign_range(aData);
	myBackBuffer = myGrid;
	mySideSize = aSideSize;
	myVelocity.clear();
	myVelocity.resize(aData.size(), 0);
}

// Following https://matthias-research.github.io/pages/tenMinutePhysics/20-heightFieldWater.pdf
// Not sure how to arrive at the discretization of the wave equeation
void WaterSim::Update(float aDelta, float aWaveSpeed, float aCellSize, float aVelDamping)
{
	const float flowAccelScale = (aWaveSpeed * aWaveSpeed) / (aCellSize * aCellSize);

	std::swap(myGrid, myBackBuffer);
	for(int y = 0; y < mySideSize; y++)
	{
		for (int x = 0; x < mySideSize; x++)
		{
			const int leftX = std::max(x - 1, 0);
			const int rightX = std::min(x + 1, mySideSize - 1);
			const int topY = std::min(y + 1, mySideSize - 1);
			const int bottomY = std::max(y - 1, 0);

			const int currInd = y * mySideSize + x;
			const int leftInd = y * mySideSize + leftX;
			const int rightInd = y * mySideSize + rightX;
			const int topInd = topY * mySideSize + x;
			const int bottomInd = bottomY * mySideSize + x;
			
			const float currHeight = myBackBuffer[currInd];
			const float leftHeight = myBackBuffer[leftInd];
			const float rightHeight = myBackBuffer[rightInd];
			const float topHeight = myBackBuffer[topInd];
			const float bottomHeight = myBackBuffer[bottomInd];

			// liquid tries to immediately balance out, so find out how much a cell is missing
			const float deltaVolume = (leftHeight + rightHeight + topHeight + bottomHeight)
				- 4 * currHeight;
			// in order for simulation to be stable/not ping-pong, using a scale that
			// slows down the acceleration of flow
			const float accel = flowAccelScale * deltaVolume;
			float& velocity = myVelocity[currInd];
			velocity += aDelta * accel;
			velocity *= aVelDamping;
			myGrid[currInd] = currHeight + aDelta * velocity;
		}
	}
}

void WaterSim::Append(int aX, int aY, float aValue)
{
	myGrid[aY * mySideSize + aX] += aValue;
}