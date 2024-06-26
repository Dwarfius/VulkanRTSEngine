#include "Precomp.h"
#include "GraphicsDialog.h"

#include "Game.h"

#include <Graphics/Graphics.h>

void GraphicsDialog::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("Graphics State", &aIsOpen))
	{
		Graphics& graphics = *Game::GetInstance()->GetGraphics();
		
		char buffer[64];
		Utils::StringFormat(buffer, "Renderer: {}", graphics.GetTypeName());
		ImGui::Text(buffer);
		if (ImGui::BeginTabBar("TabBar"))
		{
			if (ImGui::BeginTabItem("FrameBuffers"))
			{
				DrawFrameBuffers(graphics);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("RenderPasses"))
			{
				DrawRenderPasses(graphics);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Resources"))
			{
				DrawResources(graphics);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void GraphicsDialog::DrawFrameBuffers(Graphics& aGraphics)
{
	if (ImGui::BeginTable("Frame Buffers", 5, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Color");
		ImGui::TableSetupColumn("Depth");
		ImGui::TableSetupColumn("Stencil");
		ImGui::TableSetupColumn("Size");
		ImGui::TableHeadersRow();

		std::vector<std::pair<std::string_view, FrameBuffer>> frameBuffers = Graphics::DebugAccess::GetFrameBuffers(aGraphics);

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
					std::sort(frameBuffers.begin(), frameBuffers.end(), 
						[isAscending](const auto& aLeft, const auto& aRight) 
					{
						return isAscending 
							? aLeft.first < aRight.first
							: aLeft.first > aRight.first;
					});
					break;
					// Sorting only makes sense for the name, so skip the rest
				}
			}
			// Note: not resetting the SpecsDirty flag because we don't cache
			// sort results between frames. So every frame we need to maintain
			// current sorting info.
		}

		auto PrintIndexedAttachment = [](const FrameBuffer::Attachment& anAttachment, uint8_t anIndex)
		{
			char buffer[128];
			switch (anAttachment.myType)
			{
			case FrameBuffer::AttachmentType::None:
				break;
			case FrameBuffer::AttachmentType::Texture:
				Utils::StringFormat(buffer, "{}. Texture - {}", anIndex,
					ITexture::Format::kNames[anAttachment.myFormat]);
				ImGui::Text(buffer);
				break;
			case FrameBuffer::AttachmentType::RenderBuffer:
				Utils::StringFormat(buffer, "{}. Render Buffer - {}", anIndex,
					ITexture::Format::kNames[anAttachment.myFormat]);
				ImGui::Text(buffer);
				break;
			default:
				ASSERT(false);
			}
		};

		auto PrintAttachment = [](const FrameBuffer::Attachment& anAttachment)
		{
			char buffer[128];
			switch (anAttachment.myType)
			{
			case FrameBuffer::AttachmentType::None:
				ImGui::Text("None");
				break;
			case FrameBuffer::AttachmentType::Texture:
				Utils::StringFormat(buffer, "Texture - {}",
					ITexture::Format::kNames[anAttachment.myFormat]);
				ImGui::Text(buffer);
				break;
			case FrameBuffer::AttachmentType::RenderBuffer:
				Utils::StringFormat(buffer, "Render Buffer - {}",
					ITexture::Format::kNames[anAttachment.myFormat]);
				ImGui::Text(buffer);
				break;
			default:
				ASSERT(false);
			}
		};
		
		for (const auto& [name, frameBuffer] : frameBuffers)
		{
			ImGui::TableNextRow();

			if (ImGui::TableNextColumn())
			{
				ImGui::Text(name.data());
			}

			if (ImGui::TableNextColumn())
			{
				for(uint8_t i=0; i<FrameBuffer::kMaxColorAttachments; i++)
				{
					PrintIndexedAttachment(frameBuffer.myColors[i], i);
				}
			}

			if (ImGui::TableNextColumn())
			{
				PrintAttachment(frameBuffer.myDepth);
			}

			if (ImGui::TableNextColumn())
			{
				PrintAttachment(frameBuffer.myStencil);
			}

			if (ImGui::TableNextColumn())
			{
				if (frameBuffer.mySize == FrameBuffer::kFullScreen)
				{
					ImGui::Text("Fullscreen");
				}
				else
				{
					char buffer[32];
					Utils::StringFormat(buffer, "[{},{}]", frameBuffer.mySize.x, frameBuffer.mySize.y);
					ImGui::Text(buffer);
				}
			}
		}
		ImGui::EndTable();
	}
}

void GraphicsDialog::DrawRenderPasses(Graphics& aGraphics)
{
	if (ImGui::BeginTable("Render Passes", 3, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("UBO Count");
		ImGui::TableSetupColumn("Total UBO Size");
		ImGui::TableHeadersRow();

		std::vector<RenderPass*> renderPasses = Graphics::DebugAccess::GetRenderPasses(aGraphics);
		struct RenderPassInfo
		{
			const RenderPass* myPass;
			size_t myCount;
			size_t myTotalSize;
		};
		std::vector<RenderPassInfo> renderPassInfos;
		renderPassInfos.reserve(renderPasses.size());
		std::ranges::transform(renderPasses, std::back_inserter(renderPassInfos), 
			[](const RenderPass* aPass) 
		{
			return RenderPassInfo{ aPass, aPass->GetUBOCount(), aPass->GetUBOTotalSize() };
		});

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
					std::sort(renderPassInfos.begin(), renderPassInfos.end(),
						[isAscending](const auto& aLeft, const auto& aRight)
					{
						return isAscending
							? aLeft.myPass->GetTypeName() < aRight.myPass->GetTypeName()
							: aLeft.myPass->GetTypeName() > aRight.myPass->GetTypeName();
					});
					break;
				case 1:
					std::sort(renderPassInfos.begin(), renderPassInfos.end(),
						[isAscending](const auto& aLeft, const auto& aRight)
					{
						return isAscending
							? aLeft.myCount < aRight.myCount
							: aLeft.myCount > aRight.myCount;
					});
					break;
				case 2:
					std::sort(renderPassInfos.begin(), renderPassInfos.end(),
						[isAscending](const auto& aLeft, const auto& aRight)
					{
						return isAscending
							? aLeft.myTotalSize < aRight.myTotalSize
							: aLeft.myTotalSize > aRight.myTotalSize;
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

		char buffer[64];
		for (const RenderPassInfo& info : renderPassInfos)
		{
			ImGui::TableNextRow();

			if (ImGui::TableNextColumn())
			{
				ImGui::Text(info.myPass->GetTypeName().data());
			}

			if (ImGui::TableNextColumn())
			{
				Utils::StringFormat(buffer, "{}", info.myCount);
				ImGui::Text(buffer);
			}

			if (ImGui::TableNextColumn())
			{
				Utils::StringFormat(buffer, "{}", info.myTotalSize);
				ImGui::Text(buffer);
			}
		}
		ImGui::EndTable();
	}
}

void GraphicsDialog::DrawResources(Graphics& aGraphics)
{
	if (ImGui::BeginTable("Resources", 4, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Id");
		ImGui::TableSetupColumn("Type");
		ImGui::TableSetupColumn("Status");
		ImGui::TableSetupColumn("Resource Discarded");
		ImGui::TableHeadersRow();

		std::vector<const GPUResource*> resources = Graphics::DebugAccess::GetResources(aGraphics);

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
		ImGui::EndTable();
	}
}