#pragma once

#include "Interfaces/ITexture.h"

struct FrameBuffer
{
	enum class AttachmentType : uint8_t
	{
		None,
		Texture,
		RenderBuffer
	};

	struct Attachment
	{
		AttachmentType myType;
		ITexture::Format myFormat;
	};

	constexpr static uint8_t kMaxColorAttachments = 8;
	Attachment myColors[kMaxColorAttachments]{
		{ AttachmentType::None, ITexture::Format::SNorm_R }
	};

	Attachment myDepth{ AttachmentType::None, ITexture::Format::SNorm_R };
	Attachment myStencil{ AttachmentType::None, ITexture::Format::SNorm_R };

	constexpr static glm::ivec2 kFullScreen{ -1, -1 };
	glm::ivec2 mySize = kFullScreen;
};