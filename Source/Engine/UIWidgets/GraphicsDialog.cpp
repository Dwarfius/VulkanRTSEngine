#include "Precomp.h"
#include "GraphicsDialog.h"

#include "Game.h"

#include <Graphics/Graphics.h>

void GraphicsDialog::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("Graphics State", &aIsOpen))
	{
		Graphics& graphics = *Game::GetInstance()->GetGraphics();
		std::vector<const GPUResource*> resources = Graphics::DebugAccess::GetResources(graphics);
		// TODO: add which renderer in use (even though I have GL only)
		// TODO: add FrameBuffer table
		// TODO: add RenderPass table
		if (ImGui::BeginTable("Resources", 4, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Id");
			ImGui::TableSetupColumn("Type");
			ImGui::TableSetupColumn("Status");
			ImGui::TableSetupColumn("Resource Discarded");
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
							[isAscending](const GPUResource* aLeft, const GPUResource* aRight) 
						{
							return isAscending
								? aLeft->GetResourceId() < aRight->GetResourceId()
								: aLeft->GetResourceId() > aRight->GetResourceId();
						});
						break;
					case 1:
						std::sort(resources.begin(), resources.end(),
							[isAscending](const GPUResource* aLeft, const GPUResource* aRight)
						{
							return isAscending
								? aLeft->GetTypeName() < aRight->GetTypeName()
								: aLeft->GetTypeName() > aRight->GetTypeName();
						});
						break;
					case 2:
						std::sort(resources.begin(), resources.end(),
							[isAscending](const GPUResource* aLeft, const GPUResource* aRight)
						{
							return isAscending
								? aLeft->GetState() < aRight->GetState()
								: aLeft->GetState() > aRight->GetState();
						});
						break;
					case 3:
						std::sort(resources.begin(), resources.end(),
							[isAscending](const GPUResource* aLeft, const GPUResource* aRight)
						{
							return isAscending
								? aLeft->GetResource().IsValid() < aRight->GetResource().IsValid()
								: aLeft->GetResource().IsValid() > aRight->GetResource().IsValid();
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

			char buffer[128];
			for (const GPUResource* res : resources)
			{
				ImGui::TableNextRow();

				if (ImGui::TableNextColumn())
				{
					Utils::StringFormat(buffer, "{}", res->GetResourceId());
					ImGui::Text(buffer);
				}

				if (ImGui::TableNextColumn())
				{
					ImGui::Text(res->GetTypeName().data());
				}

				if (ImGui::TableNextColumn())
				{
					switch (res->GetState())
					{
					case GPUResource::State::Error:
						ImGui::TextColored(ImVec4(255, 0, 0, 255), "Error");
						break;
					case GPUResource::State::Invalid:
						ImGui::TextColored(ImVec4(255, 0, 0, 255), "Invalid");
						break;
					case GPUResource::State::PendingCreate:
						ImGui::TextColored(ImVec4(255, 255, 0, 255), "Pending Create");
						break;
					case GPUResource::State::PendingUpload:
						ImGui::TextColored(ImVec4(255, 255, 0, 255), "Pending Upload");
						break;
					case GPUResource::State::PendingUnload:
						ImGui::TextColored(ImVec4(255, 255, 0, 255), "Pending Unload");
						break;
					case GPUResource::State::Valid:
						ImGui::TextColored(ImVec4(0, 255, 0, 255), "Valid");
						break;
					default:
						ASSERT(false);
					}
				}

				if (ImGui::TableNextColumn())
				{
					const bool isValid = res->GetResource().IsValid();
					if (isValid)
					{
						ImGui::TextColored(ImVec4(255, 255, 0, 255), "Kept");
					}
					else
					{
						ImGui::TextColored(ImVec4(0, 255, 0, 255), "Discarded");
					}
				}
			}
		}
		ImGui::EndTable();
	}
	ImGui::End();
}