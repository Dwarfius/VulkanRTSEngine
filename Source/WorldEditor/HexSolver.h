#pragma once

#include <Graphics/UniformAdapterRegister.h>
#include <Engine/Components/ComponentBase.h>
#include <Graphics/Descriptor.h>

class Game;
template<class T> class Handle;
class GameObject;

class HexSolver
{
	constexpr static int kMaxSize = 100;
public:
	void Init(Game& aGame);
	void Update(Game& aGame, float aDeltaTime);

private:
	void InitGrid(Game& aGame);

	void Solve();

	std::vector<bool> myGrid;
	std::vector<Handle<GameObject>> myGameObjects;
	glm::ivec2 myStart{};
	glm::ivec2 myEnd{};
	int mySize = 10;
	int mySteps = 0;
	float myObstChance = 0.1f;
};

class HexComponent : public ComponentBase
{
public:
	std::string_view GetName() const final { return "HexComponent"; }
	bool myIsWall = false;
	bool myIsStart = false;
	bool myIsEnd = false;
	bool myIsPath = false;
};

class TintAdapter : RegisterUniformAdapter<TintAdapter>
{
public:
	constexpr static std::string_view kName = "TintAdapter";
	inline static const Descriptor ourDescriptor{
		{ Descriptor::UniformType::Vec3 }
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};