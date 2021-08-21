#pragma once

#include <Core/StaticString.h>
#include <Graphics/FrameBuffer.h>

// the engine provides a default render buffer, that
// generic render passes output to
struct DefaultFrameBuffer
{
	constexpr static StaticString kName = "Default";
	constexpr static FrameBuffer kDescriptor{
		{ 
			{ FrameBuffer::AttachmentType::Texture, ITexture::Format::UNorm_RGBA } 
		},
		{ FrameBuffer::AttachmentType::RenderBuffer, ITexture::Format::Depth32F }
	};
	constexpr static uint8_t kColorInd = 0;
};