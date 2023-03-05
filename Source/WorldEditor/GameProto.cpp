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

	HandleInput();
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
}

void GameProto::HandleInput()
{
	if (myPos.y > 0u && Input::GetKey(Input::Keys::S))
	{
		myPos.y -= 1;
	}
	if (myPos.y < mySize - 1u && Input::GetKey(Input::Keys::W))
	{
		myPos.y += 1;
	}
	if (myPos.x > 0u && Input::GetKey(Input::Keys::A))
	{
		myPos.x -= 1;
	}
	if (myPos.x < mySize - 1u && Input::GetKey(Input::Keys::D))
	{
		myPos.x += 1;
	}

	bool destroy = false;
	glm::uvec2 target;
	if (myPos.y > 1u && Input::GetKey(Input::Keys::Down))
	{
		destroy = true;
		target = myPos + glm::uvec2{ 0, -1 };
	}
	if (myPos.y < mySize - 2u && Input::GetKey(Input::Keys::Up))
	{
		destroy = true;
		target = myPos + glm::uvec2{ 0, 1 };
	}
	if (myPos.x > 1u && Input::GetKey(Input::Keys::Left))
	{
		destroy = true;
		target = myPos + glm::uvec2{ -1, 0 };
	}
	if (myPos.x < mySize - 2u && Input::GetKey(Input::Keys::Right))
	{
		destroy = true;
		target = myPos + glm::uvec2{ 1, 0 };
	}

	if (destroy)
	{
		DestroyAt(target);
	}
}

void GameProto::DestroyAt(glm::uvec2 aPos)
{
	const size_t index = aPos.y * mySize + aPos.x;
	Handle<GameObject>& go = myGameObjects[index];
	go->GetRenderable()->myVO.SetTexture(myTextures[kVoid]);
}