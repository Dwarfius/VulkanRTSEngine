#include "Precomp.h"
#include "LinearPoissonSampling.h"
#include "StaticVector.h"

void LinearPoissonSampling::Generate(std::vector<glm::vec2>& aRes, float aRadius)
{
	aRes.clear();

	// a bunch of utilities that we'll need
	//std::mt19937 generator(std::random_device{}());
	std::mt19937 generator(1337);
	// [-1, 1)
	std::uniform_real_distribution<float> sampler(-1.f, std::nextafter(1.f, 0.f));

	auto GenPoint = [&]() 
	{
		const float x = sampler(generator);
		const float y = sampler(generator);
		return glm::vec2 { x, y	};
	};

	auto GetTiled = [](glm::vec2 aPoint)
	{
		if (aPoint.x < -1)
		{
			aPoint.x += 2;
		}
		else if (aPoint.x > 1)
		{
			aPoint.x -= 2;
		}

		if (aPoint.y < -1)
		{
			aPoint.y += 2;
		}
		else if (aPoint.y > 1)
		{
			aPoint.y -= 2;
		}
		return aPoint;
	};

	Grid grid{ aRadius };
	std::vector<int> candidates;

	const glm::vec2 point = GenPoint();
	aRes.push_back(point);
	candidates.push_back(0);
	grid.Add(point, 0);

	std::vector<int> neighbors;
	while (!candidates.empty())
	{
		const int randIndex = generator() % candidates.size();
		const int candidateInd = candidates[randIndex];
		candidates[randIndex] = candidates[candidates.size() - 1];
		candidates.pop_back();

		glm::vec2 candidatePoint = aRes[candidateInd];

		ScallopedRegion region(candidatePoint, aRadius * 2, aRadius * 4);

		FindNeighbors(neighbors, candidatePoint, aRadius * 8, grid, aRes);

		for (int neighborInd : neighbors)
		{
			glm::vec2 neighborPoint = aRes[neighborInd];
			glm::vec2 tiledNeighbor = candidatePoint + GetTiled(neighborPoint - candidatePoint);

			// what is that if?
			region.SubtractDisk(tiledNeighbor, neighborInd < candidateInd ? aRadius * 4 : aRadius * 2);
		}

		while (!region.IsEmpty())
		{
			glm::vec2 newPoint = region.Sample(generator);
			glm::vec2 tiledPoint = GetTiled(newPoint);

			aRes.push_back(tiledPoint);
			const int lastInd = static_cast<int>(aRes.size() - 1);
			candidates.push_back(lastInd);
			grid.Add(tiledPoint, lastInd);

			region.SubtractDisk(newPoint, aRadius * 2);
		}
	}
}

void LinearPoissonSampling::FindNeighbors(std::vector<int>& aNeighbors, glm::vec2 aPoint, float aRadius, 
	const Grid& aGrid, std::vector<glm::vec2>& aPoints)
{
	auto GetTiledGrid = [size = aGrid.myGridSize](int coord)
	{
		if (coord >= size)
		{
			return coord - size;
		}
		else if (coord < 0)
		{
			return coord + size;
		}
		return coord;
	};

	auto GetTiledDomain = [](glm::vec2 aPoint)
	{
		if (aPoint.x < -1)
		{
			aPoint.x += 2;
		}
		else if (aPoint.x > 1)
		{
			aPoint.x -= 2;
		}

		if (aPoint.y < -1)
		{
			aPoint.y += 2;
		}
		else if (aPoint.y > 1)
		{
			aPoint.y -= 2;
		}
		return aPoint;
	};

	auto GetDistSqr = [&](glm::vec2 aP1, glm::vec2 aP2)
	{
		return glm::length2(GetTiledDomain(aP2 - aP1));
	};

	aNeighbors.clear();
	const float distSqr = aRadius * aRadius;
	const int N = glm::min(
		static_cast<int>(glm::ceil(aRadius / aGrid.myCellSize)), 
		aGrid.myGridSize >> 1
	);
	const glm::ivec2 indices = aGrid.GetIndex(aPoint);
	for (int y = indices.y - N; y <= indices.y + N; y++)
	{
		const int tiledY = GetTiledGrid(y);
		for (int x = indices.x - N; x <= indices.x + N; x++)
		{
			const int tiledX = GetTiledGrid(x);
			for (int index : aGrid.myCells[tiledY * aGrid.myGridSize + tiledX])
			{
				if (index == Grid::kUnset)
				{
					break;
				}
				else
				{
					if (GetDistSqr(aPoint, aPoints[index]) < distSqr)
					{
						aNeighbors.push_back(index);
					}
				}
			}
		}
	}
}

LinearPoissonSampling::ScallopedSector::ScallopedSector(glm::vec2 anOrigin, float aStartAngle, float anEndAngle,
	glm::vec2 aPoint1, float aRad1, float aSign1,
	glm::vec2 aPoint2, float aRad2, float aSign2)
	: myOrigin(anOrigin)
	, myStartAngle(aStartAngle)
	, myEndAngle(anEndAngle)
	, myArcs{
		Arc(aPoint1, anOrigin, aStartAngle, aRad1, aSign1),
		Arc(aPoint2, anOrigin, aStartAngle, aRad2, aSign2)
	}
{
	myArea = CalcAreaToAngle(anEndAngle);
}

float LinearPoissonSampling::ScallopedSector::CalcAreaToAngle(float anAngle) const
{
	const float underInner = Arc::IntegralOfDistToCircle(anAngle - myArcs[0].myTheta,
		myArcs[0].myDist, myArcs[0].myRadius, myArcs[0].mySign) - myArcs[0].myIntegralAtStart;
	const float underOuter = Arc::IntegralOfDistToCircle(anAngle - myArcs[1].myTheta,
		myArcs[1].myDist, myArcs[1].myRadius, myArcs[1].mySign) - myArcs[1].myIntegralAtStart;
	return underOuter - underInner;
}

float LinearPoissonSampling::ScallopedSector::CalcAngleForArea(float anArea, std::mt19937& aGenerator) const
{
	float start = myStartAngle;
	float end = myEndAngle;

	// why do we take random? why not start at 0.5f?
	// (0, 1)
	std::uniform_real_distribution<float> openSampler(
		std::nextafter(0.f, 1.f), std::nextafter(1.f, 0.f)
	);
	float curr = start + (end - start) * openSampler(aGenerator);
	for (uint8_t i = 0; i < 10; i++)
	{
		const float areaForAngle = CalcAreaToAngle(curr);
		if (areaForAngle < anArea)
		{
			start = curr;
			curr = (curr + end) * 0.5f;
		}
		else
		{
			end = curr;
			curr = (start + curr) * 0.5f;
		}
	}
	return curr;
}

float LinearPoissonSampling::ScallopedSector::DistToCurve(float anAngle, int anIndex) const
{
	const Arc& arc = myArcs[anIndex];
	const float alpha = anAngle - arc.myTheta;
	const float sinAlpha = glm::sin(alpha);
	const float t = arc.myRadSquared - arc.myDistSquared * sinAlpha * sinAlpha;
	if (t < 0)
	{
		return arc.myDist * glm::cos(alpha);
	}
	return arc.myDist * glm::cos(alpha) + arc.mySign * glm::sqrt(t);
}

void LinearPoissonSampling::ScallopedSector::DistToCircle(float anAngle, glm::vec2 aPoint, float aRadius, float& aD1, float& aD2) const
{
	const glm::vec2 diff = aPoint - myOrigin;
	const float distSqr = glm::length2(diff);
	const float theta = glm::atan(diff.y, diff.x);
	const float alpha = anAngle - theta;
	const float sinAlpha = glm::sin(alpha);
	const float xSqr = aRadius * aRadius - distSqr * sinAlpha * sinAlpha;
	if (xSqr >= 0)
	{
		const float a = glm::sqrt(distSqr) * glm::cos(alpha);
		const float x = glm::sqrt(xSqr);
		aD1 = a - x;
		aD2 = a + x;
	}
	else
	{
		aD1 = aD2 = -10000;
	}
}

glm::vec2 LinearPoissonSampling::ScallopedSector::Sample(std::mt19937& aGenerator) const
{
	// (0, 1)
	std::uniform_real_distribution<float> openSampler(
		std::nextafter(0.f, 1.f), std::nextafter(1.f, 0.f)
	); 
	// [0, 1)
	std::uniform_real_distribution<float> openEndSampler(
		0, std::nextafter(1.f, 0.f)
	);
	const float angle = CalcAngleForArea(myArea * openEndSampler(aGenerator), aGenerator);
	const float d1 = DistToCurve(angle, 0);
	const float d2 = DistToCurve(angle, 1);
	const float d = glm::sqrt(d1 * d1 + (d2 * d2 - d1 * d1) * openSampler(aGenerator));
	return { myOrigin.x + glm::cos(angle) * d, myOrigin.y + glm::sin(angle) * d };
}

void LinearPoissonSampling::ScallopedSector::SubtractDisk(glm::vec2 aPoint, float aRadius, 
	std::vector<ScallopedSector>& aSectors) const
{
	StaticVector<float, 8> angles;
	angles.PushBack(myStartAngle);

	const glm::vec2 diff = aPoint - myOrigin;
	const float dist = glm::length(diff);

	if (aRadius < dist)
	{
		const float theta = glm::atan(diff.y, diff.x);
		const float x = glm::sqrt(dist * dist - aRadius * aRadius);
		const float alpha = glm::asin(aRadius / dist);

		float angle = CanonizeAngle(theta + alpha);
		if (myStartAngle < angle && angle < myEndAngle
			&& DistToCurve(angle, 0) < x && x < DistToCurve(angle, 1))
		{
			angles.PushBack(angle);
		}

		angle = CanonizeAngle(theta - alpha);
		if (myStartAngle < angle && angle < myEndAngle
			&& DistToCurve(angle, 0) < x && x < DistToCurve(angle, 1))
		{
			angles.PushBack(angle);
		}
	}

	for (uint8_t i = 0; i < 2; i++)
	{
		const Arc& arc = myArcs[i];
		const glm::vec2 diffToArc = aPoint - arc.myPoint;
		const float distToArc = glm::length(diffToArc);

		if (distToArc > std::numeric_limits<float>::epsilon())
		{
			const float invDistToArc = 1 / distToArc;
			const float x = (distToArc * distToArc - aRadius * aRadius + arc.myRadSquared)
				* invDistToArc * 0.5f;
			const float k = arc.myRadSquared - x * x;

			if (k > 0)
			{
				const float y = glm::sqrt(k);
				const glm::vec2 normDiffToArc = diffToArc * invDistToArc;
				const float vxx = normDiffToArc.x * x;
				const float vxy = normDiffToArc.x * y;
				const float vyx = normDiffToArc.y * x;
				const float vyy = normDiffToArc.y * y;

				float angle = CanonizeAngle(glm::atan(
					arc.myPoint.y + vyx + vxy - myOrigin.y, arc.myPoint.x + vxx - vyy - myOrigin.x)
				);
				if (myStartAngle < angle && angle < myEndAngle)
				{
					angles.PushBack(angle);
				}

				angle = CanonizeAngle(glm::atan(
					arc.myPoint.y + vyx - vxy - myOrigin.y, arc.myPoint.x + vxx + vyy - myOrigin.x)
				);
				if (myStartAngle < angle && angle < myEndAngle)
				{
					angles.PushBack(angle);
				}
			}
		}
	}

	std::sort(angles.begin() + 1, angles.end());
	angles.PushBack(myEndAngle);

	for (uint8_t i = 1; i < angles.GetSize(); i++)
	{
		const float a1 = angles[i - 1];
		const float a2 = angles[i];

		const float midA = (a1 + a2) * 0.5f;
		const float inner = DistToCurve(midA, 0);
		const float outer = DistToCurve(midA, 1);

		float d1, d2;
		DistToCircle(midA, aPoint, aRadius, d1, d2);
		ASSERT(d1 <= d2);

		if (d2 < inner || d1 > outer)
		{
			aSectors.push_back({ 
				myOrigin, a1, a2, 
				myArcs[0].myPoint, myArcs[0].myRadius, myArcs[0].mySign, 
				myArcs[1].myPoint, myArcs[1].myRadius, myArcs[1].mySign
			});
		}
		else
		{
			if (inner < d1)
			{
				aSectors.push_back({
					myOrigin, a1, a2,
					myArcs[0].myPoint, myArcs[0].myRadius, myArcs[0].mySign,
					aPoint, aRadius, -1
				});
			}
			if (d2 < outer)
			{
				aSectors.push_back({
					myOrigin, a1, a2,
					aPoint, aRadius, 1,
					myArcs[1].myPoint, myArcs[1].myRadius, myArcs[1].mySign
				});
			}
		}
	}
}

void LinearPoissonSampling::ScallopedRegion::SubtractDisk(glm::vec2 aPoint, float aRadius)
{
	myArea = 0;
	std::vector<ScallopedSector> newSectors;
	std::vector<ScallopedSector> subtractedSectors;
	for (const ScallopedSector& sector : mySectors)
	{
		subtractedSectors.clear();
		sector.SubtractDisk(aPoint, aRadius, subtractedSectors);

		for (const ScallopedSector& subtracted : subtractedSectors)
		{
			if (subtracted.myArea > myMinArea)
			{
				myArea += subtracted.myArea;
				if (!newSectors.empty())
				{
					ScallopedSector& last = newSectors[newSectors.size() - 1];
					// try expanding last arc
					if (last.myEndAngle == subtracted.myStartAngle
						&& Arc::AreSimilar(last.myArcs[0], subtracted.myArcs[0])
						&& Arc::AreSimilar(last.myArcs[1], subtracted.myArcs[1]))
					{
						last.myEndAngle = subtracted.myEndAngle;
						last.myArea = last.CalcAreaToAngle(last.myEndAngle);
						continue;
					}
				}
				newSectors.push_back(subtracted);
			}
		}
	}
	mySectors = std::move(newSectors);
}

glm::vec2 LinearPoissonSampling::ScallopedRegion::Sample(std::mt19937& aGenerator)
{
	ASSERT_STR(!mySectors.empty(), "No sectors left!");
	// [0, 1)
	std::uniform_real_distribution<float> openEndSampler(
		0, std::nextafter(1.f, 0.f)
	);
	float area = myArea * openEndSampler(aGenerator);
	for (ScallopedSector& sector : mySectors)
	{
		if (area < sector.myArea)
		{
			return sector.Sample(aGenerator);
		}
		area -= sector.myArea;
	}
	ASSERT(false);
	return {};
}