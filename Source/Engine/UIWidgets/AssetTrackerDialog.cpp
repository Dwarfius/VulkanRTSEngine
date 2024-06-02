#include "Precomp.h"
#include "AssetTrackerDialog.h"

#include "Game.h"

#include <Core/Resources/AssetTracker.h>

void AssetTrackerDialog::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("AssetTracker State", &aIsOpen))
	{
		AssetTracker& assetTracker = Game::GetInstance()->GetAssetTracker();
		std::vector<Handle<Resource>> resources = AssetTracker::DebugAccess::AccessResources(assetTracker);
		char buffer[128];
		if (ImGui::BeginTable("Resources", 4, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Id");
			ImGui::TableSetupColumn("Status");
			ImGui::TableSetupColumn("Type");
			ImGui::TableSetupColumn("Path");
			ImGui::TableHeadersRow();
			
			ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
			if (sortSpecs && sortSpecs->SpecsDirty)
			{
				for (int i = 0; i < sortSpecs->SpecsCount; i++)
				{
					const ImGuiTableColumnSortSpecs& sortSpec = sortSpecs->Specs[i];
					const bool isAscending = sortSpec.SortDirection == ImGuiSortDirection_Ascending;
					switch (sortSpec.ColumnIndex)
					{
					case 0:
						std::sort(resources.begin(), resources.end(),
							[isAscending](const auto& aLeft, const auto& aRight)
						{
							return isAscending
								? aLeft->GetId() < aRight->GetId()
								: aLeft->GetId() > aRight->GetId();
						});
						break;
					case 1:
						std::sort(resources.begin(), resources.end(),
							[isAscending](const auto& aLeft, const auto& aRight)
						{
							return isAscending
								? aLeft->GetState() < aRight->GetState()
								: aLeft->GetState() > aRight->GetState();
						});
						break;
					case 2:
						std::sort(resources.begin(), resources.end(),
							[isAscending](const auto& aLeft, const auto& aRight)
						{
							return isAscending
								? aLeft->GetTypeName() < aRight->GetTypeName()
								: aLeft->GetTypeName() > aRight->GetTypeName();
						});
						break;
					case 3:
						std::sort(resources.begin(), resources.end(),
							[isAscending](const auto& aLeft, const auto& aRight)
						{
							return isAscending
								? aLeft->GetPath() < aRight->GetPath()
								: aLeft->GetPath() > aRight->GetPath();
						});
						break;
					default:
						ASSERT(false);
					}
				}
				// Note: not resetting the SpecsDirty flag because we don't cache
				// sort results between frames. So every frame we need to maintain
				// current sorting info.
			}

			for (const Handle<Resource>& resHandle : resources)
			{
				ImGui::TableNextRow();

				const Resource* res = resHandle.Get();
				if (ImGui::TableNextColumn())
				{
					Utils::StringFormat(buffer, "{}", res->GetId());
					ImGui::Text(buffer);
				}

				if (ImGui::TableNextColumn())
				{
					switch (res->GetState())
					{
					case Resource::State::Uninitialized:
						ImGui::TextColored(ImVec4(255, 0, 0, 255), "Uninitialized");
						break;
					case Resource::State::PendingLoad:
						ImGui::TextColored(ImVec4(255, 255, 0, 255), "Pending Load");
						break;
					case Resource::State::Ready:
						ImGui::TextColored(ImVec4(0, 255, 0, 255), "Ready");
						break;
					case Resource::State::Error:
						ImGui::TextColored(ImVec4(255, 0, 0, 255), "Error");
						break;
					default:
						ASSERT(false);
					}
				}

				if (ImGui::TableNextColumn())
				{
					ImGui::Text(res->GetTypeName().data());
				}

				if (ImGui::TableNextColumn())
				{
					std::string_view path = res->GetPath();
					if (!path.empty())
					{
						ImGui::Text(path.data());
					}
					else
					{
						ImGui::Text("Dynamic Resource");
					}
				}
			}
		}
		ImGui::EndTable();
	}
	ImGui::End();
}