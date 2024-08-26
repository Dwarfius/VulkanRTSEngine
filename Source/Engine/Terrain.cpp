#include "Precomp.h"
#include "Terrain.h"

#include "GameObject.h"

#include <Core/Algos/DiamondSquareAlgo.h>
#include <Physics/PhysicsShapes.h>
#include <Graphics/Resources/Texture.h>

void Terrain::Load(Handle<Texture> aTexture, float aStep, float anYScale)
{
	myYScale = anYScale;
	myStep = aStep;
	myTexture = aTexture;

	myTexture->ExecLambdaOnLoad([this, anYScale](const Resource* aRes) {
		const Texture* texture = static_cast<const Texture*>(aRes);
		ASSERT_STR(texture->GetState() != Resource::State::Error, "Failed to load terrain texture!");
		ASSERT_STR(texture->GetFormat() == Texture::Format::UNorm_R, "Texture must have single channel format!");

		using PixelType = unsigned char;
		constexpr float kMaxPixelVal = std::numeric_limits<PixelType>::max();

		myWidth = texture->GetWidth();
		myHeight = texture->GetHeight();

		std::vector<float> heights;
		heights.resize(myWidth * myHeight);
		const PixelType* pixels = texture->GetPixels();
		for (uint32_t y = 0; y < myHeight; y++)
		{
			for (uint32_t x = 0; x < myWidth; x++)
			{
				const PixelType pixel = pixels[y * myWidth + x];
				const float scaledHeight = pixel * anYScale;
				myMinHeight = glm::min(scaledHeight, myMinHeight);
				myMaxHeight = glm::max(scaledHeight, myMaxHeight);
				heights[y * myWidth + x] = scaledHeight;
			}
		}

		myHeightfield = std::make_shared<PhysicsShapeHeightfield>
			(myWidth, myHeight, std::move(heights), myMinHeight, myMaxHeight);
	});
}

void Terrain::Generate(glm::uvec2 aSize, float aStep, float anYScale)
{
	ASSERT_STR(Utils::CountSetBits(aSize.x) == 1
		&& Utils::CountSetBits(aSize.y) == 1, "Size must be power of 2!");
	ASSERT_STR(aSize.x > 2 && aSize.y > 2, "Small sizes aren't supported by the texture!");
	myWidth = aSize.x;
	myHeight = aSize.y;

	myYScale = anYScale;
	myStep = aStep;

	// The algo we use has specific size constaints,
	// so we'll need to generate in a larger buffer, then extract
	// the area we'll be rendering
	std::vector<float> heights;
	{
		const size_t gridSize = std::max<size_t>(myWidth, myHeight) + 1;
		std::vector<float> workingState;
		workingState.resize(gridSize * gridSize);
		DiamondSquareAlgo dsAlgo(0, static_cast<uint32_t>(gridSize), 0.f, myYScale);
		dsAlgo.Generate(workingState.data());

		heights.resize(myWidth * myHeight);
		for (uint32_t y = 0; y < myHeight; y++)
		{
			for (uint32_t x = 0; x < myWidth; x++)
			{
				heights[y * myWidth + x] = workingState[y * gridSize + x];
			}
		}
	}

	{
		myTexture = new Texture();
		myTexture->SetWidth(myWidth);
		myTexture->SetHeight(myHeight);
		// TODO: this isn't the most efficient, but ah well
		// because we store single bytes, we need to 
		// downscale/quantize floats to chars
		myTexture->SetFormat(Texture::Format::UNorm_R);
		
		using PixelType = unsigned char;
		constexpr float kMaxPixelVal = std::numeric_limits<typename PixelType>::max();
		PixelType* pixels = new PixelType[myWidth * myHeight];
		for (uint32_t y = 0; y < myHeight; y++)
		{
			for (uint32_t x = 0; x < myWidth; x++)
			{
				const float floatHeight = heights[y * myWidth + x];
				myMinHeight = glm::min(floatHeight, myMinHeight);
				myMaxHeight = glm::max(floatHeight, myMaxHeight);
				const PixelType charHeight = static_cast<PixelType>(
					floatHeight / myYScale * kMaxPixelVal
				);
				pixels[y * myWidth + x] = charHeight;
			}
		}
		myTexture->SetPixels(pixels);
	}

	myHeightfield = std::make_shared<PhysicsShapeHeightfield>
		(myWidth, myHeight, std::move(heights), myMinHeight, myMaxHeight);
}

void Terrain::GenerateNormals()
{
	ASSERT_STR(myHeightfield, "No data to generate normals from!");

	struct NormalSum
	{
		glm::vec3 myNormal;
		uint8_t myCount = 0;
	};
	std::vector<NormalSum> normals;
	normals.resize(myWidth * myHeight);
	for (uint32_t y = 0; y < myHeight - 1; y++)
	{
		for (uint32_t x = 0; x < myWidth - 1; x++)
		{
			const uint32_t v1Ind = y * myWidth + x;
			const uint32_t v2Ind = y * myWidth + x + 1;
			const uint32_t v3Ind = (y + 1) * myWidth + x;
			const uint32_t v4Ind = (y + 1) * myWidth + x + 1;

			const glm::vec3 a{ x, myHeightfield->GetHeights()[v1Ind], y };
			const glm::vec3 b{ x + 1, myHeightfield->GetHeights()[v2Ind], y };
			const glm::vec3 c{ x, myHeightfield->GetHeights()[v3Ind], y + 1 };
			const glm::vec3 d{ x + 1, myHeightfield->GetHeights()[v4Ind], y + 1 };

			const glm::vec3 n1 = glm::normalize(glm::cross(c - a, b - a));
			const glm::vec3 n2 = glm::normalize(glm::cross(d - c, b - c));

			normals[v1Ind].myNormal += n1;
			normals[v1Ind].myCount += 1;

			normals[v2Ind].myNormal += n1 + n2;
			normals[v2Ind].myCount += 2;

			normals[v3Ind].myNormal += n1 + n2;
			normals[v3Ind].myCount += 2;

			normals[v4Ind].myNormal += n2;
			normals[v4Ind].myCount += 1;
		}
	}

	myNormals.resize(normals.size());
	for(uint32_t i=0; i<normals.size(); i++)
	{
		myNormals[i] = normals[i].myNormal / static_cast<float>(normals[i].myCount);
	}
}

float Terrain::GetHeight(glm::vec2 aLocalPos) const
{
	ASSERT_STR(myHeightfield, "Terrain hasn't finished initalizing!");

	// finding the relative position
	float x = aLocalPos.x / myStep;
	float z = aLocalPos.y / myStep;

	ASSERT_STR(x >= 0 && x < myWidth && z >= 0 && z < myHeight, 
		"Incorrect coords passed! Were they world space?");

	// getting the actual vert indices
	const uint32_t xMin = static_cast<uint32_t>(glm::floor(x));
	const uint32_t xMax = glm::min(xMin + 1, myWidth - 1);
	const uint32_t zMin = static_cast<uint32_t>(glm::floor(z));
	const uint32_t zMax = glm::min(zMin + 1, myHeight - 1);

	// getting vertices for lerping
	const float v0 = GetHeightAtVert(xMin, zMin);
	const float v1 = GetHeightAtVert(xMin, zMax);
	const float v2 = GetHeightAtVert(xMax, zMin);
	const float v3 = GetHeightAtVert(xMax, zMax);

	// getting the height
	x -= xMin;
	z -= zMin;
	const float botHeight = glm::mix(v0, v2, x);
	const float topHeight = glm::mix(v1, v3, x);
	return glm::mix(botHeight, topHeight, z);
}

glm::vec3 Terrain::GetNormal(glm::vec2 aLocalPos) const
{
	ASSERT_STR(myHeightfield, "Terrain hasn't finished initalizing!");

	// finding the relative position
	float x = aLocalPos.x / myStep;
	float z = aLocalPos.y / myStep;

	ASSERT_STR(x >= 0 && x < myWidth && z >= 0 && z < myHeight,
		"Incorrect coords passed! Were they world space?");

	// getting the actual vert indices
	const uint32_t xMin = static_cast<uint32_t>(glm::floor(x));
	const uint32_t xMax = glm::min(xMin + 1, myWidth - 1);
	const uint32_t zMin = static_cast<uint32_t>(glm::floor(z));
	const uint32_t zMax = glm::min(zMin + 1, myHeight - 1);

	// getting vertices for lerping
	const glm::vec3 n0 = myNormals[zMin * myWidth + xMin];
	const glm::vec3 n1 = myNormals[zMax * myWidth + xMin];
	const glm::vec3 n2 = myNormals[zMin * myWidth + xMax];
	const glm::vec3 n3 = myNormals[zMax * myWidth + xMax];

	// getting the height
	x -= xMin;
	z -= zMin;
	const glm::vec3 botNorm = glm::mix(n0, n2, x);
	const glm::vec3 topNorm = glm::mix(n1, n3, x);
	return glm::mix(botNorm, topNorm, z);
}

void Terrain::PushHeightLevelColor(float aHeightLevel, glm::vec3 aColor)
{
	ASSERT_STR(myLevelsCount < kMaxHeightLevels, 
		"Overflow, terrain only supports {} levels!", kMaxHeightLevels);

	myHeightLevelColors[myLevelsCount++] = {
		.myColor = aColor,
		.myHeight = aHeightLevel
	};
	std::sort(
		myHeightLevelColors.begin(), 
		myHeightLevelColors.begin() + myLevelsCount,
		[](HeightLevelColor aLeft, HeightLevelColor aRight) 
		{
			return aLeft.myHeight < aRight.myHeight;
		}
	);
}

void Terrain::RemoveHeightLevelColor(uint8_t anInd)
{
	ASSERT_STR(anInd < myLevelsCount, "Out of bounds removal!");
	myLevelsCount--;
	std::swap(myHeightLevelColors[anInd], myHeightLevelColors[myLevelsCount]);
	std::sort(
		myHeightLevelColors.begin(),
		myHeightLevelColors.begin() + myLevelsCount,
		[](HeightLevelColor aLeft, HeightLevelColor aRight)
		{
			return aLeft.myHeight < aRight.myHeight;
		}
	);
}

float Terrain::GetHeightAtVert(uint32_t aX, uint32_t aY) const
{
	return myHeightfield->GetHeights()[aY * myWidth + aX];
}