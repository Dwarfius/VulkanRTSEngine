#pragma once

class WaterSim
{
public:
	void SeedState(std::span<float> aData, int aSideSize);

	// For stable results use aWaveSpeed < aCellSize
	void Update(float aDelta, float aWaveSpeed, float aCellSize, float aVelDamping);

	std::span<const float> GetHeightfield() const { return myGrid; }
	int GetSideSize() const { return mySideSize; }

	void Append(int aX, int aY, float aValue);

private:
	std::vector<float> myGrid;
	std::vector<float> myBackBuffer;
	std::vector<float> myVelocity;
	int mySideSize;
};