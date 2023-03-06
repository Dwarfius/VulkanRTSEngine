#include "Precomp.h"
#include "GameProto.h"

#include "DefaultAssets.h"

#include <Engine/Game.h>
#include <Core/Resources/AssetTracker.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>
#include <Engine/Input.h>

#include <Graphics/Resources/Texture.h>
#include <Graphics/Camera.h>

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
			ImGui::End();
		}
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

	// reposition the camera to see all
	const glm::vec3 focusPoint = (myGameObjects.front()->GetWorldTransform().GetPos()
		+ myGameObjects.back()->GetWorldTransform().GetPos()) / 2.f;
	aGame.GetCamera()->GetTransform().SetPos(
		focusPoint + glm::vec3{ 0, mySize * kSize, mySize * kSize }
	);
	aGame.GetCamera()->GetTransform().LookAt(focusPoint);

	// initailize selection
	if (!oldSize)
	{
		SetColor({ 0, 0 }, kSelected);
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