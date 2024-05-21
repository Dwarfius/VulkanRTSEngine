#include "Precomp.h"
#include "GameProto.h"

#include "DefaultAssets.h"

#include <Engine/Game.h>
#include <Core/Resources/AssetTracker.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>
#include <Engine/Input.h>

#include <Graphics/Camera.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>

GameProto::GameProto(Game& aGame)
{
	AssetTracker& tracker = aGame.GetAssetTracker();
	myPipeline = tracker.GetOrCreate<Pipeline>("Engine/Unlit.ppl");
	for (size_t i=0; i<kColorTypes; i++)
	{
		Texture* texture = new Texture();
		texture->SetHeight(1);
		texture->SetWidth(1);
		texture->SetPixels(new uint8_t[3]{ kColors[i].x, kColors[i].y, kColors[i].z});
		//tracker.AssignDynamicId(*texture);
		myTextures[i] = texture;
	}
}

void GameProto::Update(Game& aGame, DefaultAssets& aAssets, float aDelta)
{
	{
		std::lock_guard lock(aGame.GetImGUISystem().GetMutex());
		if (ImGui::Begin("Game Proto"))
		{
			ImGui::InputScalar("Size", ImGuiDataType_U8, &mySize);
			if (ImGui::Button("Generate"))
			{
				Generate(aGame, aAssets);
			}

			if (myNodes.size() > 0 && ImGui::Button("Update Water"))
			{
				UpdateWater();
			}
		}
		ImGui::End();
	}

	if (myNodes.size())
	{
		HandleInput();
	}
}

void GameProto::Generate(Game& aGame, DefaultAssets& aAssets)
{
	myNodes.resize(mySize * mySize);
	std::ranges::fill(myNodes, Node{ kGround });

	size_t oldSize = myGameObjects.size();
	myGameObjects.resize(myNodes.size());
	const float offset = 10.f + mySize * kSize;
	for (size_t i = oldSize; i < myGameObjects.size(); i++)
	{
		const size_t x = i % mySize;
		const size_t y = i / mySize;
		const Transform transf({ x * kSize - offset, 0, y * -kSize }, {}, { kSize, kSize, kSize });
		Handle<GameObject> newGO = new GameObject(transf);
		newGO->CreateRenderable();

		VisualObject& vo = newGO->GetRenderable()->myVO;
		vo.SetPipeline(myPipeline);
		vo.SetModel(aAssets.GetBox());

		myGameObjects[i] = newGO;

		aGame.AddGameObject(newGO);
	}

	for (size_t i = 0; i < myGameObjects.size(); i++)
	{
		const Handle<Texture>& colorText = myTextures[myNodes[i].myValue];
		myGameObjects[i]->GetRenderable()->myVO.SetTexture(colorText);
	}
	GenerateRivers();

	// reposition the camera to see all
	const glm::vec3 focusPoint = (myGameObjects.front()->GetWorldTransform().GetPos()
		+ myGameObjects.back()->GetWorldTransform().GetPos()) / 2.f;
	aGame.GetCamera()->GetTransform().SetPos(
		focusPoint + glm::vec3{ 0, mySize * kSize, mySize * kSize }
	);
	aGame.GetCamera()->GetTransform().LookAt(focusPoint);

	// initailize selection
	SetColor({ 0, 0 }, kSelected);
}

void GameProto::GenerateRivers()
{
	// try to generate winding/snaking rivers
	// every turn the chance to go the other way increases
	enum Move
	{
		Left = 0,
		Up = 1,
		Right = 2, // opposite -> 2 - Move
		Down = 3, // opposite -> 4 - Move
		None = 4
	};
	constexpr std::array<float, 4> kBaseChances{
		0.5f, // "Forward"
		0.3f, // "Opposite" turn
		0.2f, // "Same" turn
		0.f // End
	};
	auto GetOpposite = [](Move aMove) {
		ASSERT(aMove != None);
		return aMove == Left || aMove == Right ?
			Move(2 - aMove) : Move(4 - aMove);
	};
	auto GetNewMoves = [&](Move aNewDir, Move aLastTurn) {
		std::array<Move, 4> moves{
			aNewDir,
			GetOpposite(aLastTurn),
			aLastTurn,
			None
		};
		ASSERT(moves[0] != moves[1]);
		ASSERT(moves[0] != moves[2]);
		ASSERT(moves[1] != moves[2]);
		return moves;
	};
	constexpr float kChanceIncrease = 0.01f;
	constexpr float kChanceDecrease = 0.02f;
	
	const uint8_t riverCount = mySize / 2;
	std::random_device dev;
	std::mt19937 engine(dev());
	std::uniform_int_distribution<uint32_t> spawnDistrib(0, mySize - 1);
	std::array<Move, 4> moves{ // gets shuffled by river generation
		Up,
		Right,
		Left,
		None // always the same
	};
	for (uint8_t riverInd = 0; riverInd < riverCount; riverInd++)
	{
		std::array currChances = kBaseChances;
		glm::uvec2 currPos{
			spawnDistrib(engine),
			spawnDistrib(engine)
		};
		SetColor(currPos, kWater);
		uint8_t length = mySize;
		std::uniform_real_distribution<float> turnDistrib;
		while (length)
		{
			float moveRoll = turnDistrib(engine);
			Move pickedMove = None;
			for (uint8_t i = 0; i < currChances.size(); i++)
			{
				if (moveRoll < currChances[i])
				{
					pickedMove = moves[i];
					break;
				}

				moveRoll -= currChances[i];
			}

			switch (pickedMove)
			{
			case Up:
				currPos.y += 1;
				break;
			case Down:
				currPos.y -= 1;
				break;
			case Right:
				currPos.x += 1;
				break;
			case Left:
				currPos.x -= 1;
				break;
			default:
				break;
			}
			if (pickedMove != None
				&& currPos.x >= 0 && currPos.x < mySize
				&& currPos.y >= 0 && currPos.y < mySize)
			{
				SetColor(currPos, kWater);

				for (uint8_t i = 0; i < currChances.size(); i++)
				{
					if (pickedMove == moves[i])
					{
						currChances[i] -= kChanceDecrease;
					}
					else
					{
						currChances[i] += kChanceIncrease;
					}
				}
				if (pickedMove != moves[0]) // are we making a turn?
				{
					const bool randomTurn = turnDistrib(engine) > 0.5f;
					moves = GetNewMoves(pickedMove, randomTurn ? moves[0] : GetOpposite(moves[0]));
				}
				length--;
			}
			else
			{
				length = 0;
			}
		}
	}
}

void GameProto::UpdateWater()
{
	std::vector<glm::uvec2> toUpdate;
	toUpdate.reserve(mySize * mySize / 4);

	for (uint32_t y = 0; y < mySize; y++)
	{
		for (uint32_t x = 0; x < mySize; x++)
		{
			Node& node = GetNode({ x, y });
			if (node.myValue == kWater)
			{
				toUpdate.push_back({ x, y });
			}
		}
	}

	for (glm::uvec2 pos : toUpdate)
	{
		const uint32_t x = pos.x;
		const uint32_t y = pos.y;
		if (x > 0 && GetNode({ x - 1, y }).myValue == kVoid)
		{
			SetColor({ x - 1, y }, kWater);
		}
		if (x < (uint32_t)(mySize - 1) && GetNode({ x + 1, y }).myValue == kVoid)
		{
			SetColor({ x + 1, y }, kWater);
		}
		if (y > 0 && GetNode({ x, y - 1 }).myValue == kVoid)
		{
			SetColor({ x, y - 1 }, kWater);
		}
		if (y < (uint32_t)(mySize - 1) && GetNode({ x, y + 1 }).myValue == kVoid)
		{
			SetColor({ x, y + 1 }, kWater);
		}
	}
}

void GameProto::HandleInput()
{
	glm::uvec2 oldPos = myPos;
	const bool ignoreTerrain = Input::GetKey(Input::Keys::Shift);
	if (myPos.y > 0u && Input::GetKeyPressed(Input::Keys::S))
	{
		if (CanTraverseTo(myPos + glm::uvec2{ 0, -1 }) || ignoreTerrain)
		{
			myPos.y -= 1;
		}
	}
	if (myPos.y < mySize - 1u && Input::GetKeyPressed(Input::Keys::W))
	{
		if (CanTraverseTo(myPos + glm::uvec2{ 0, 1 }) || ignoreTerrain)
		{
			myPos.y += 1;
		}
	}
	if (myPos.x > 0u && Input::GetKeyPressed(Input::Keys::A))
	{
		if (CanTraverseTo(myPos + glm::uvec2{ -1, 0 }) || ignoreTerrain)
		{
			myPos.x -= 1;
		}
	}
	if (myPos.x < mySize - 1u && Input::GetKeyPressed(Input::Keys::D))
	{
		if (CanTraverseTo(myPos + glm::uvec2{ 1, 0 }) || ignoreTerrain)
		{
			myPos.x += 1;
		}
	}

	if (myPos != oldPos)
	{
		SetColor(myPos, kSelected);
		SetColor(oldPos, GetNode(oldPos).myValue);
	}

	if (Input::GetKeyPressed(Input::Keys::Space))
	{
		SetColor(myPos, kVoid);
	}

	if (Input::GetKeyPressed(Input::Keys::E))
	{
		SetColor(myPos, kWater);
	}
}

GameProto::Node& GameProto::GetNode(glm::uvec2 aPos)
{
	const size_t index = aPos.y * mySize + aPos.x;
	return myNodes[index];
}

void GameProto::SetColor(glm::uvec2 aPos, uint8_t aColorInd)
{
	const size_t index = aPos.y * mySize + aPos.x;
	myGameObjects[index]->GetRenderable()->myVO.SetTexture(myTextures[aColorInd]);
	if (aColorInd != kSelected)
	{
		myNodes[index].myValue = aColorInd;
	}
}

bool GameProto::CanTraverseTo(glm::uvec2 aPos)
{
	Node& node = GetNode(aPos);
	return node.myValue != kVoid && node.myValue != kWater;
}