#pragma once

// TODO: Would be interesting to try out 
// "Maximal Poisson disk sampling: an improved version of Bridson’s algorithm"
// by Martin Roberts, found at
// https://extremelearning.com.au/an-improved-version-of-bridsons-algorithm-n-for-poisson-disc-sampling/
// it seems much simpler, less implementation to write, less trig funcs

// Linear poison sampling based on Daniel Dunbar & Greg Humphreys
// "A spatial data structure for fast Poisson-disk sample generation" 
// - https://dl.acm.org/doi/10.1145/1179352.1141915. Example can be found here:
// https://github.com/ddunbar/PDSample
class LinearPoissonSampling
{
	struct Arc
	{
		glm::vec2 myPoint;
		float myRadius;
		float mySign;
		float myTheta;
		float myDist;
		float myIntegralAtStart;
		float myRadSquared;
		float myDistSquared;

		Arc(glm::vec2 aPoint, glm::vec2 anOrigin, float anAngle, float aRadius, float aSign)
			: myPoint(aPoint)
			, myRadius(aRadius)
			, mySign(aSign)
		{
			//ASSERT(InDomain(myPoint));
			glm::vec2 offset = myPoint - anOrigin;
			myTheta = glm::atan(offset.y, offset.x);
			myDist = glm::length(offset);
			myRadSquared = myRadius * myRadius;
			myDistSquared = myDist * myDist;
			myIntegralAtStart = IntegralOfDistToCircle(anAngle - myTheta, myDist, myRadius, mySign);
		}

		static bool InDomain(glm::vec2 aP)
		{
			return aP.x >= -1 && aP.x <= 1 && aP.y >= -1 && aP.y <= 1;
		}

		static float IntegralOfDistToCircle(float anX, float aD, float anR, float aK)
		{
			if (anR < std::numeric_limits<float>::epsilon())
			{
				return 0;
			}

			const float sinX = glm::sin(anX);
			const float dSinX = aD * sinX;
			const float y = glm::clamp(dSinX / anR, -1.f, 1.f);
			const float theta = glm::asin(y);
			return 0.5f * (
				((anX + aK * theta) * anR 
					+ aK * glm::cos(theta) * dSinX) * anR 
				+ aD * glm::cos(anX) * dSinX
			);
		}

		static bool AreSimilar(const Arc& aLeft, const Arc& aRight)
		{
			return aLeft.myPoint == aRight.myPoint
				&& aLeft.myRadius == aRight.myRadius
				&& aLeft.mySign == aRight.mySign;
		}
	};

	struct ScallopedSector
	{
		ScallopedSector(glm::vec2 anOrigin, float aStartAngle, float anEndAngle,
			glm::vec2 aPoint1, float aRad1, float aSign1,
			glm::vec2 aPoint2, float aRad2, float aSign2);

		float CalcAreaToAngle(float anAngle) const;
		float CalcAngleForArea(float anArea, std::mt19937& aGenerator) const;
		float DistToCurve(float anAngle, int anIndex) const;
		void DistToCircle(float anAngle, glm::vec2 aPoint, float aRadius, float& aD1, float& aD2) const;
		glm::vec2 Sample(std::mt19937& aGenerator) const;

		float CanonizeAngle(float anAngle) const
		{
			float delta = glm::mod(anAngle - myStartAngle, glm::two_pi<float>());
			if (delta < 0)
			{
				delta += glm::two_pi<float>();
			}
			return myStartAngle + delta;
		}

		void SubtractDisk(glm::vec2 aPoint, float aRadius, std::vector<ScallopedSector>& aSectors) const;

		Arc myArcs[2];
		glm::vec2 myOrigin;
		float myStartAngle, myEndAngle;
		float myArea;
	};

	struct ScallopedRegion
	{
		ScallopedRegion(glm::vec2 aPoint, float aMinRadius, float aMaxRadius, float aMinArea = 0.00000001f)
			: myMinArea(aMinArea)
		{
			mySectors.push_back({ 
				aPoint, 0, glm::two_pi<float>(), 
				aPoint, aMinRadius, 1, 
				aPoint, aMaxRadius, 1 
			});
			myArea = mySectors[0].myArea;
		}

		void SubtractDisk(glm::vec2 aPoint, float aRadius);
		glm::vec2 Sample(std::mt19937& aGenerator);
		bool IsEmpty() const { return mySectors.empty(); }

		std::vector<ScallopedSector> mySectors;
		float myArea;
		float myMinArea;
	};

	struct Grid
	{
		constexpr static uint8_t kMaxPoints = 9;
		constexpr static int kUnset = -1;
		std::vector<std::array<int, kMaxPoints>> myCells;
		int myGridSize;
		float myCellSize;

		Grid(float aRadius)
		{
			constexpr float kDomainSize = 2.f;
			myGridSize = glm::max(
				static_cast<int>(glm::ceil(kDomainSize / (4 * aRadius))),
				static_cast<int>(kDomainSize)
			);
			myCellSize = kDomainSize / myGridSize;
			myCells.resize(myGridSize * myGridSize);
			for (auto& array : myCells)
			{
				for (uint8_t i = 0; i < kMaxPoints; i++)
				{
					array[i] = kUnset;
				}
			}
		}

		void Add(glm::vec2 aPoint, int anIndex)
		{
			const glm::ivec2 index = GetIndex(aPoint);
			auto& array = myCells[index.y * myGridSize + index.x];
			for (uint8_t i = 0; i < kMaxPoints; i++)
			{
				if (array[i] == kUnset)
				{
					array[i] = anIndex;
					return;
				}
			}
			ASSERT_STR(false, "Overflown grid cell capacity!");
		}

		glm::ivec2 GetIndex(glm::vec2 aPoint) const
		{
			ASSERT_STR(aPoint.x >= -1 && aPoint.x < 1, "Outside of range!");
			ASSERT_STR(aPoint.y >= -1 && aPoint.y < 1, "Outside of range!");
			return glm::ivec2{
				static_cast<int>(glm::floor(0.5f * (aPoint.x + 1) * myGridSize)),
				static_cast<int>(glm::floor(0.5f * (aPoint.y + 1) * myGridSize))
			};
		}
	};

public:
	// Generates points in the [-1, 1]^2 range
	static void Generate(std::vector<glm::vec2>& aRes, float aRadius);

private:
	static void FindNeighbors(std::vector<int>& aNeighbors, glm::vec2 aPoint, float aRadius, 
		const Grid& aGrid, std::vector<glm::vec2>& aPoints);
};