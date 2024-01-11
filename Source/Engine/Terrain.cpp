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

	const size_t gridSize = std::max<size_t>(myWidth, myHeight) + 1;
	std::vector<float> heights;
	heights.resize(gridSize * gridSize);
	
	DiamondSquareAlgo dsAlgo(0, static_cast<uint32_t>(gridSize), 0.f, myYScale);
   	dsAlgo.Generate(heights.data());

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
				const float floatHeight = heights[y * gridSize + x];
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

float Terrain::GetHeight(glm::vec3 aLocalPos) const
{
	ASSERT_STR(!myHeightfield, "Terrain hasn't finished initalizing!");

	// finding the relative position
	float x = aLocalPos.x / myStep;
	float z = aLocalPos.z / myStep;

	ASSERT_STR(x >= 0 && x < myWidth && z >= 0 && z < myHeight, 
		"Incorrect coords passed! Were they world space?");

	// getting the actual vert indices
	const uint32_t xMin = static_cast<uint32_t>(glm::floor(x));
	const uint32_t xMax = glm::min(xMin + 1, myWidth - 1);
	const uint32_t zMin = static_cast<uint32_t>(glm::floor(z));
	const uint32_t zMax = glm::min(zMin + 1, myHeight - 1);

	// getting vertices for lerping
	float v0 = GetHeightAtVert(xMin, zMin);
	float v1 = GetHeightAtVert(xMin, zMax);
	float v2 = GetHeightAtVert(xMax, zMin);
	float v3 = GetHeightAtVert(xMax, zMax);

	// getting the height
	x -= xMin;
	z -= zMin;
	const float botHeight = glm::mix(v0, v2, x);
	const float topHeight = glm::mix(v1, v3, x);
	return glm::mix(botHeight, topHeight, z);
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
	return myHeightfield->GetHeights()[aY * myHeight + aX];
}