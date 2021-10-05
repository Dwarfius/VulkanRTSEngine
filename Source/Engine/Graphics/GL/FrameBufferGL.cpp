#include "Precomp.h"
#include "FrameBufferGL.h"

#include "Graphics/GL/GraphicsGL.h"
#include "Graphics/GL/TextureGL.h"

FrameBufferGL::FrameBufferGL(GraphicsGL& aGraphics, const FrameBuffer& aDescriptor)
{
	glGenFramebuffers(1, &myBuffer);
	Bind();

	const uint32_t width = static_cast<uint32_t>(aGraphics.GetWidth());
	const uint32_t height = static_cast<uint32_t>(aGraphics.GetHeight());

	for (uint8_t i = 0; i < FrameBuffer::kMaxColorAttachments; i++)
	{
		FrameBuffer::Attachment colorAttachment = aDescriptor.myColors[i];
		switch (colorAttachment.myType)
		{
		case FrameBuffer::AttachmentType::None: break;
		case FrameBuffer::AttachmentType::Texture:
			myColorAttachments[i] = CreateTextureAttachment(
				width, height,
				colorAttachment.myFormat, GL_COLOR_ATTACHMENT0 + i
			);
			break;
		case FrameBuffer::AttachmentType::RenderBuffer:
			myColorAttachments[i] = CreateRenderBufferAttachment(
				width, height,
				colorAttachment.myFormat, GL_COLOR_ATTACHMENT0 + i
			);
			break;
		default:
			ASSERT(false);
		}
	}

	const ITexture::Format depthFormat = aDescriptor.myDepth.myFormat;
	const GLenum depthAttachment = (depthFormat == ITexture::Format::Depth24_Stencil8
								|| depthFormat == ITexture::Format::Depth32F_Stencil8) ?
									GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
	switch (aDescriptor.myDepth.myType)
	{
	case FrameBuffer::AttachmentType::None: break;
	case FrameBuffer::AttachmentType::Texture:
		myDepthAttachment = CreateTextureAttachment(
			width, height,
			depthFormat, depthAttachment
		);
		break;
	case FrameBuffer::AttachmentType::RenderBuffer:
		myDepthAttachment = CreateRenderBufferAttachment(
			width, height,
			depthFormat, depthAttachment
		);
		break;
	default:
		ASSERT(false);
	}

	ASSERT_STR(aDescriptor.myStencil.myType == FrameBuffer::AttachmentType::None
		|| depthAttachment == GL_DEPTH_ATTACHMENT, "Invalid Frame Buffer setup - attempting "
		"to setup a stencil when there is already a depth&stencil resource created");
	switch (aDescriptor.myStencil.myType)
	{
	case FrameBuffer::AttachmentType::None: break;
	case FrameBuffer::AttachmentType::Texture:
		myStencilAttachment = CreateTextureAttachment(
			width, height,
			aDescriptor.myStencil.myFormat, GL_STENCIL_ATTACHMENT
		);
		break;
	case FrameBuffer::AttachmentType::RenderBuffer:
		myStencilAttachment = CreateRenderBufferAttachment(
			width, height,
			aDescriptor.myStencil.myFormat, GL_STENCIL_ATTACHMENT
		);
		break;
	default:
		ASSERT(false);
	}

	ASSERT_STR(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
		"Did not result in a complete frame buffer!");
}

FrameBufferGL::~FrameBufferGL()
{
	glDeleteFramebuffers(1, &myBuffer);

	for (const Attachment& attachment : myColorAttachments)
	{
		if (attachment.myRes)
		{
			if (attachment.myIsTexture)
			{
				glDeleteTextures(1, &attachment.myRes);
			}
			else
			{
				glDeleteRenderbuffers(1, &attachment.myRes);
			}
		}
	}

	if (myDepthAttachment.myRes)
	{
		if (myDepthAttachment.myIsTexture)
		{
			glDeleteTextures(1, &myDepthAttachment.myRes);
		}
		else
		{
			glDeleteRenderbuffers(1, &myDepthAttachment.myRes);
		}
	}

	if (myStencilAttachment.myRes)
	{
		if (myStencilAttachment.myIsTexture)
		{
			glDeleteTextures(1, &myStencilAttachment.myRes);
		}
		else
		{
			glDeleteRenderbuffers(1, &myStencilAttachment.myRes);
		}
	}
}

FrameBufferGL& FrameBufferGL::operator=(FrameBufferGL&& aOther) noexcept
{
	// Since we can never end up with an un-initialized FrameBufferGL
	// we don't need to clean-up on move - the exchange will leave
	// the moved-from in valid state, and once it goes out of scope
	// it'll be cleaned up - this saves additional checks in various areas
	myBuffer = std::exchange(aOther.myBuffer, myBuffer);
	for (uint8_t i = 0; i < FrameBuffer::kMaxColorAttachments; i++)
	{
		myColorAttachments[i] = std::exchange(aOther.myColorAttachments[i], myColorAttachments[i]);
	}
	myDepthAttachment = std::exchange(aOther.myDepthAttachment, myDepthAttachment);
	myStencilAttachment = std::exchange(aOther.myStencilAttachment, myStencilAttachment);
	return *this;
}

void FrameBufferGL::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, myBuffer);
}

uint32_t FrameBufferGL::GetColorTexture(uint8_t anIndex) const
{
	ASSERT(anIndex < FrameBuffer::kMaxColorAttachments);
	ASSERT(myColorAttachments[anIndex].myRes != 0);
	ASSERT(myColorAttachments[anIndex].myIsTexture);
	return myColorAttachments[anIndex].myRes;
}

uint32_t FrameBufferGL::GetDepthTexture() const
{
	ASSERT(myDepthAttachment.myRes != 0);
	ASSERT(myDepthAttachment.myIsTexture);
	return myDepthAttachment.myRes;
}

uint32_t FrameBufferGL::GetStencilTexture() const
{
	ASSERT(myStencilAttachment.myRes != 0);
	ASSERT(myStencilAttachment.myIsTexture);
	return myStencilAttachment.myRes;
}

void FrameBufferGL::OnResize(GraphicsGL& aGraphics)
{
	const uint32_t width = static_cast<uint32_t>(aGraphics.GetWidth());
	const uint32_t height = static_cast<uint32_t>(aGraphics.GetHeight());

	auto ResizeTexture = [=](uint32_t aTexture, ITexture::Format aFormat) {
		glBindTexture(GL_TEXTURE_2D, aTexture);

		const GLint internFormat = TextureGL::TranslateInternalFormat(aFormat);
		const GLenum format = TextureGL::TranslateFormat(aFormat);
		const GLenum pixelType = TextureGL::DeterminePixelDataType(aFormat);
		
		glTexImage2D(GL_TEXTURE_2D, 0, internFormat,
			width, height,
			0, format, pixelType, nullptr
		);
	};

	auto ResizeRenderBuffer = [=](uint32_t aBuffer, ITexture::Format aFormat) {
		glBindRenderbuffer(GL_RENDERBUFFER, aBuffer);

		const GLint internFormat = TextureGL::TranslateInternalFormat(aFormat);
		glRenderbufferStorage(GL_RENDERBUFFER, internFormat, width, height);
	};

	for (Attachment& attachment : myColorAttachments)
	{
		if (attachment.myRes)
		{
			if (attachment.myIsTexture)
			{
				ResizeTexture(attachment.myRes, attachment.myFormat);
			}
			else
			{
				ResizeRenderBuffer(attachment.myRes, attachment.myFormat);
			}
		}
	}

	if (myDepthAttachment.myRes)
	{
		if (myDepthAttachment.myIsTexture)
		{
			ResizeTexture(myDepthAttachment.myRes, myDepthAttachment.myFormat);
		}
		else
		{
			ResizeRenderBuffer(myDepthAttachment.myRes, myDepthAttachment.myFormat);
		}
	}

	if (myStencilAttachment.myRes)
	{
		if (myStencilAttachment.myIsTexture)
		{
			ResizeTexture(myStencilAttachment.myRes, myStencilAttachment.myFormat);
		}
		else
		{
			ResizeRenderBuffer(myStencilAttachment.myRes, myStencilAttachment.myFormat);
		}
	}
}

FrameBufferGL::Attachment FrameBufferGL::CreateTextureAttachment(uint32_t aWidth, uint32_t aHeight, ITexture::Format aFormat, uint32_t anAttachment)
{
	GLuint texture;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);

	const GLint internFormat = TextureGL::TranslateInternalFormat(aFormat);
	const GLenum format = TextureGL::TranslateFormat(aFormat);
	const GLenum pixelType = TextureGL::DeterminePixelDataType(aFormat);
	glTexImage2D(GL_TEXTURE_2D, 0, internFormat,
		aWidth, aHeight,
		0, format, pixelType, nullptr
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, anAttachment,
		GL_TEXTURE_2D, texture, 0);

	return { texture, aFormat, true };
}

FrameBufferGL::Attachment FrameBufferGL::CreateRenderBufferAttachment(uint32_t aWidth, uint32_t aHeight, ITexture::Format aFormat, uint32_t anAttachment)
{
	GLuint renderBuffer;
	glGenRenderbuffers(1, &renderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);

	const GLint internFormat = TextureGL::TranslateInternalFormat(aFormat);
	glRenderbufferStorage(GL_RENDERBUFFER, internFormat, aWidth, aHeight);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, anAttachment, GL_RENDERBUFFER, renderBuffer);

	return { renderBuffer, aFormat, false };
}