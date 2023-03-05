#pragma once

#include <Core/RefCounted.h>

class Game;
class DefaultAssets;
class GameObject;
class Pipeline;
class Texture;

class GameProto
{
	constexpr static float kSize = 2.f;

	struct Node
	{
		uint8_t myValue;
	};

	constexpr static int8_t kSelected = 0;
	constexpr static int8_t kVoid = 1;
	constexpr static int8_t kWater = 2;
	constexpr static int8_t kGround = 3;
	constexpr static glm::u8vec3 kColors[]{
		{0xFF, 0xD8, 0}, // selected
		{0, 0, 0}, // void
		{0, 0, 0xFF}, // water
		{0xFF, 0xFF, 0xFF}, // ground
		{0xFF, 0xFF, 0}
	};
	constexpr static uint8_t kColorTypes = std::extent_v<decltype(kColors)>;
public:
	GameProto(Game& aGame);
	void Update(Game& aGame, DefaultAssets& aAssets, float aDelta);

private:
	void Generate(Game& aGame, DefaultAssets& aAssets);
	void HandleInput();
	Node& GetNode(glm::uvec2 aPos);
	void SetColor(glm::uvec2 aPos, uint8_t aColorInd);

	std::vector<Handle<GameObject>> myGameObjects;
	std::vector<Node> myNodes;
	glm::uvec2 myPos = { 0, 0 };
	uint8_t mySize = 4;
	Handle<Pipeline> myPipeline;
	Handle<Texture> myTextures[kColorTypes];
};