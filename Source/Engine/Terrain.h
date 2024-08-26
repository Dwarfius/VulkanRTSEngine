#pragma once

#include <Core/RefCounted.h>

class PhysicsShapeHeightfield;
class Texture;

class Terrain
{
public:
	// How many height levels we can support at the moment
	constexpr static uint8_t kMaxHeightLevels = 5;
	struct HeightLevelColor
	{
		glm::vec3 myColor;
		float myHeight;
	};

	void Load(Handle<Texture> aTexture, float aStep, float anYScale);
	void Generate(glm::uvec2 aSize, float aStep, float anYScale);

	void GenerateNormals();
	
	// pos is in local space
	float GetHeight(glm::vec2 aLocalPos) const;
	glm::vec3 GetNormal(glm::vec2 aLocalPos) const;

	Handle<Texture> GetTextureHandle() const { return myTexture; }

	float GetWidth() const { return (myWidth - 1) * myStep; }
	float GetDepth() const { return (myHeight - 1) * myStep; }
	float GetYScale() const { return myYScale; }

	// TODO: move this out of Engine (let individual apps do their own thing)
	uint8_t GetHeightLevelCount() const { return myLevelsCount; }
	HeightLevelColor GetHeightLevelColor(uint8_t anInd) const { return myHeightLevelColors[anInd]; }
	void PushHeightLevelColor(float aHeightLevel, glm::vec3 aColor);
	void RemoveHeightLevelColor(uint8_t anInd);

	std::shared_ptr<PhysicsShapeHeightfield> GetPhysShape() const { return myHeightfield; }

private:
	float GetHeightAtVert(uint32_t aX, uint32_t aY) const;

	Handle<Texture> myTexture;
	std::vector<glm::vec3> myNormals;

	// dimensions of the heightmap texture used
	uint32_t myWidth = 0, myHeight = 0;
	// vertical extent of the terrain
	float myMinHeight = std::numeric_limits<float>::max();
	float myMaxHeight = std::numeric_limits<float>::min();
	// distance between neighbouring terrain vertices
	float myStep = 0;
	// controls how much texture should be scaled "vertically"
	float myYScale = 0;

	std::array<HeightLevelColor, kMaxHeightLevels> myHeightLevelColors;
	uint8_t myLevelsCount = 0;

	// TODO: remove this from the terrain
	std::shared_ptr<PhysicsShapeHeightfield> myHeightfield;
};