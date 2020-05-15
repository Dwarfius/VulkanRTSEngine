#include "Precomp.h"
#include "Terrain.h"

#include "GameObject.h"
#include "Components/PhysicsComponent.h"

#include <Core/Algos/DiamondSquareAlgo.h>
#include <Physics/PhysicsShapes.h>
#include <Graphics/Resources/Texture.h>

// TODO: write own header for collision object flags
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

Terrain::Terrain()
	: myWidth(0)
	, myHeight(0)
	, myMinHeight(std::numeric_limits<float>::max())
	, myMaxHeight(std::numeric_limits<float>::min())
	, myStep(0.f)
	, myYScale(0.f)
{
}

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
		constexpr float kMaxPixelVal = std::numeric_limits<typename PixelType>::max();

		myWidth = texture->GetWidth();
		myHeight = texture->GetHeight();

		myHeightCache.resize(myWidth * myHeight);
		const PixelType* pixels = texture->GetPixels();
		for (uint32_t y = 0; y < myHeight; y++)
		{
			for (uint32_t x = 0; x < myWidth; x++)
			{
				const PixelType pixel = pixels[y * myWidth + x];
				const float scaledHeight = pixel * anYScale;
				myMinHeight = glm::min(scaledHeight, myMinHeight);
				myMaxHeight = glm::max(scaledHeight, myMaxHeight);
				myHeightCache[y * myWidth + x] = scaledHeight;
			}
		}
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
	myHeightCache.resize(gridSize * gridSize);
	
	DiamondSquareAlgo dsAlgo(0, gridSize, 0.f, myYScale);
   	dsAlgo.Generate(myHeightCache.data());

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
				const float floatHeight = myHeightCache[y * gridSize + x];
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
}

float Terrain::GetHeight(glm::vec3 pos) const
{
	ASSERT_STR(!myHeightCache.empty(), "Terrain hasn't finished initalizing!");

	// finding the relative position
	float x = pos.x / myStep;
	float y = pos.z / myStep;

	ASSERT_STR(x >= 0 && x <= myWidth - 1 && y >= 0 && y <= myHeight - 1, 
		"Incorrect coords passed! Were they world space?");

	// getting the actual vert indices
	uint32_t xMin = static_cast<uint32_t>(glm::floor(x));
	uint32_t xMax = glm::min(xMin + 1, myWidth - 1);
	uint32_t yMin = static_cast<uint32_t>(glm::floor(y));
	uint32_t yMax = glm::min(yMin + 1, myHeight - 1);

	// getting vertices for lerping
	float v0 = GetHeightAtVert(xMin, yMin);
	float v1 = GetHeightAtVert(xMin, yMax);
	float v2 = GetHeightAtVert(xMax, yMin);
	float v3 = GetHeightAtVert(xMax, yMax);

	// getting the height
	x -= xMin;
	y -= yMin;
	float botHeight = glm::mix(v0, v2, x);
	float topHeight = glm::mix(v1, v3, x);
	return glm::mix(botHeight, topHeight, y);
}

glm::vec3 Terrain::GetNormal(glm::vec3 pos) const
{
	ASSERT_STR(false, "To be implemented");
	return glm::vec3();
}

void Terrain::AddPhysicsEntity(GameObject& aGO, PhysicsWorld& aPhysWorld)
{
	myTexture->ExecLambdaOnLoad([this, &aGO, &aPhysWorld](const Resource* aRes) {
		PhysicsComponent* physComp = aGO.AddComponent<PhysicsComponent>();
		std::shared_ptr<PhysicsShapeHeightfield> terrShape = std::make_shared<PhysicsShapeHeightfield>
			(myWidth, myHeight, myHeightCache.data(), myMinHeight, myMaxHeight);

		glm::vec3 pos = aGO.GetTransform().GetPos();
		pos = terrShape->AdjustPositionForRecenter(pos);
		physComp->SetOrigin(pos);

		physComp->CreatePhysicsEntity(0, terrShape);
		physComp->GetPhysicsEntity().SetCollisionFlags(
			physComp->GetPhysicsEntity().GetCollisionFlags()
			| btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT
		);

		physComp->RequestAddToWorld(aPhysWorld);
	});
}

float Terrain::GetHeightAtVert(uint32_t x, uint32_t y) const
{
	return myHeightCache[y * myHeight + x];
}