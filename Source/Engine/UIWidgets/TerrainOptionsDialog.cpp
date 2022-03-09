#include "Precomp.h"
#include "TerrainOptionsDialog.h"

#include "Game.h"
#include "Terrain.h"

void TerrainOptionsDialog::Draw(Game& aGame, bool& aIsVisible)
{
	if (ImGui::Begin("Terrain Options", &aIsVisible))
	{
		size_t terrainCount = aGame.GetTerrainCount();
		for (uint32_t i = 0; i < terrainCount; i++)
		{
			Terrain& terrain = *aGame.GetTerrain(i);
			if (ImGui::BeginChild(i + 1, ImVec2(0, 0), true))
			{
				DrawTerrain(terrain);
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();
}

void TerrainOptionsDialog::DrawTerrain(Terrain& aTerrain)
{
	uint8_t heightLayerCount = aTerrain.GetHeightLevelCount();
	ImGui::LabelText("Layer Count", "%u", heightLayerCount);
	if (heightLayerCount < Terrain::kMaxHeightLevels)
	{
		ImGui::Separator();
		ImGui::ColorEdit3("Color", glm::value_ptr(myNewColor));
		ImGui::InputFloat("Height", &myNewHeight);
		if (ImGui::Button("Add Layer"))
		{
			aTerrain.PushHeightLevelColor(myNewHeight, myNewColor);
			myNewHeight = 0;
			myNewColor = { 1, 1, 1 };
		}
	}
	ImGui::Separator();
	for (uint8_t i = 0; i < heightLayerCount; i++)
	{
		ImGui::PushID(i);
		bool changed = false;
		Terrain::HeightLevelColor entry = aTerrain.GetHeightLevelColor(i);
		changed |= ImGui::ColorEdit3("Color", glm::value_ptr(entry.myColor));
		changed |= ImGui::InputFloat("Height", &entry.myHeight);
		ImGui::PopID();

		if (changed)
		{
			aTerrain.RemoveHeightLevelColor(i);
			aTerrain.PushHeightLevelColor(entry.myHeight, entry.myColor);
		}
	}
}