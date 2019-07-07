#include "Precomp.h"
#include "TextureGL.h"

#include <Graphics/Texture.h>

TextureGL::TextureGL()
	: GPUResource()
	, myGLTexture(0)
{
}

TextureGL::~TextureGL()
{
	if (myGLTexture)
	{
		Unload();
	}
}

void TextureGL::Bind()
{
	ASSERT_STR(myGLTexture, "Missing GL texture to bind!");
	glBindTexture(GL_TEXTURE_2D, myGLTexture);
}

void TextureGL::Create(any aDescriptor)
{
	const Texture::CreateDescriptor& desc = 
		any_cast<const Texture::CreateDescriptor&>(aDescriptor);

	ASSERT_STR(!myGLTexture, "Uploading an uploaded texture!");

	glGenTextures(1, &myGLTexture);
	glBindTexture(GL_TEXTURE_2D, myGLTexture);

	switch (desc.myWrapMode)
	{
	case GPUResource::WrapMode::Repeat:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
	case GPUResource::WrapMode::Clamp:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
	case GPUResource::WrapMode::MirroredRepeat:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
	}
	
	switch (desc.myMinFilter)
	{
	case GPUResource::Filter::Nearest:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case GPUResource::Filter::Linear:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	case GPUResource::Filter::Nearest_MipMapNearest:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		break;
	case GPUResource::Filter::Linear_MipMapNearest:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		break;
	case GPUResource::Filter::Nearest_MipMapLinear:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		break;
	case GPUResource::Filter::Linear_MipMapLinear:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		break;
	default:
		ASSERT(false);
	}
	
	switch (desc.myMagFilter)
	{
	case GPUResource::Filter::Nearest:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case GPUResource::Filter::Linear:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	default:
		ASSERT(false);
	}
}

bool TextureGL::Upload(any aDescriptor)
{
	const Texture::UploadDescriptor& desc = 
		any_cast<const Texture::UploadDescriptor&>(aDescriptor);

	ASSERT_STR(myGLTexture, "Uploading an uploaded texture!");

	// rebind it in case it's not bound
	glBindTexture(GL_TEXTURE_2D, myGLTexture);

	uint32_t format;
	switch (desc.myFormat)
	{
	case Texture::Format::UNorm_R: format = GL_R; break;
	case Texture::Format::UNorm_RG: format = GL_RG; break; // not sure about this one, need to check stbi
	case Texture::Format::UNorm_RGB:	format = GL_RGB; break;
	case Texture::Format::UNorm_RGBA: // fallthrough
	case Texture::Format::UNorm_BGRA: format = GL_RGBA; break;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, desc.myWidth, desc.myHeight, 0, format, GL_UNSIGNED_BYTE, desc.myPixels);

	if (desc.myEnableMipmaps)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	return true;
}

void TextureGL::Unload()
{
	ASSERT_STR(myGLTexture, "Attempt to free an uninitialized texture!");
	glDeleteTextures(1, &myGLTexture);
}