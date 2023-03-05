#include "Precomp.h"
#include "GameProto.h"

#include "DefaultAssets.h"

#include <Engine/Game.h>
#include <Core/Resources/AssetTracker.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>
#include <Engine/Input.h>

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
		const Transform transf({ x * kSize - offset, 0, y * kSize }, {}, { kSize, kSize, kSize });
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

	if (!oldSize)
	{
		SetColor({ 0, 0 }, kSelected);
	}
}

void GameProto::HandleInput()
{
	glm::uvec2 oldPos = myPos;
	if (myPos.y > 0u && Input::GetKeyPressed(Input::Keys::S))
	{
		myPos.y -= 1;
	}
	if (myPos.y < mySize - 1u && Input::GetKeyPressed(Input::Keys::W))
	{
		myPos.y += 1;
	}
	if (myPos.x > 0u && Input::GetKeyPressed(Input::Keys::A))
	{
		myPos.x -= 1;
	}
	if (myPos.x < mySize - 1u && Input::GetKeyPressed(Input::Keys::D))
	{
		myPos.x += 1;
	}

	if (myPos != oldPos)
	{
		SetColor(oldPos, GetNode(oldPos).myValue);
		SetColor(myPos, kSelected);
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