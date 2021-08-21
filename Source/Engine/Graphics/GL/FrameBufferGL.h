#pragma once

#include <Graphics/FrameBuffer.h>
#include <Graphics/Interfaces/ITexture.h>

class GraphicsGL;

class FrameBufferGL
{
public:
	// Changes OpenGL state
	FrameBufferGL(GraphicsGL& aGraphics, const FrameBuffer& aDescriptor);
	// Changes OpenGL state
	~FrameBufferGL();

	FrameBufferGL(const FrameBufferGL&) = delete;
	FrameBufferGL& operator=(const FrameBufferGL&) = delete;
	FrameBufferGL(FrameBufferGL&& aOther) noexcept { *this = std::move(aOther); }
	FrameBufferGL& operator=(FrameBufferGL&& aOther) noexcept;

	// Changes OpenGL state
	void Bind();

	uint32_t GetColorTexture(uint8_t anIndex) const;
	uint32_t GetDepthTexture() const;
	uint32_t GetStencilTexture() const;

	void OnResize(GraphicsGL& aGraphics);

private:
	struct Attachment
	{
		uint32_t myRes = 0;
		ITexture::Format myFormat;
		bool myIsTexture = false;
	};

	static Attachment CreateTextureAttachment(uint32_t aWidth, uint32_t aHeight, ITexture::Format aFormat, uint32_t anAttachment);
	static Attachment CreateRenderBufferAttachment(uint32_t aWidth, uint32_t aHeight, ITexture::Format aFormat, uint32_t anAttachment);

	uint32_t myBuffer = 0;
	Attachment myColorAttachments[FrameBuffer::kMaxColorAttachments];
	Attachment myDepthAttachment;
	Attachment myStencilAttachment;
};