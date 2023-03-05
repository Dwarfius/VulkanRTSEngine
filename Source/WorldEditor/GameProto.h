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

	constexpr static uint8_t kColorTypes = 3;
	constexpr static int8_t kVoid = 0;
	constexpr static int8_t kGround = 1;
	constexpr static glm::u8vec3 kColors[kColorTypes]{
		{0, 0, 0}, // void
		{0xFF, 0xFF, 0xFF}, // empty
		{0xFF, 0xFF, 0}
	};
public:
	GameProto(Game& aGame);
	void Update(Game& aGame, DefaultAssets& aAssets, float aDelta);

private:
	void Generate(Game& aGame, DefaultAssets& aAssets);
	void HandleInput();
	void DestroyAt(glm::uvec2 aPos);

	std::vector<Handle<GameObject>> myGameObjects;
	std::vector<Node> myNodes;
	glm::uvec2 myPos = { 0, 0 };
	uint8_t mySize = 4;
	Handle<Pipeline> myPipeline;
	Handle<Texture> myTextures[kColorTypes];
};