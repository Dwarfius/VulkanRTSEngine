#include "Precomp.h"
#include "TextureGL.h"

#include <Graphics/Resources/Texture.h>

TextureGL::TextureGL()
	: myGLTexture(0)
	, myWrapMode(static_cast<Texture::WrapMode>(-1))
	, myMinFilter(static_cast<Texture::Filter>(-1))
	, myMagFilter(static_cast<Texture::Filter>(-1))
{
}

void TextureGL::Bind()
{
	ASSERT_STR(myGLTexture, "Missing GL texture to bind!");
	glBindTexture(GL_TEXTURE_2D, myGLTexture);
}

void TextureGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myGLTexture, "Recreating an existing texture!");

	glGenTextures(1, &myGLTexture);
}

bool TextureGL::OnUpload(Graphics& aGraphics)
{
	ASSERT_STR(myGLTexture, "Uploading an uploaded texture!");

	// rebind it in case it's not bound
	glBindTexture(GL_TEXTURE_2D, myGLTexture);

	const Texture* texture = myResHandle.Get<const Texture>();
	UpdateTexParams(texture);

	uint32_t format;
	switch (texture->GetFormat())
	{
	case Texture::Format::UNorm_R: format = GL_R; break;
	case Texture::Format::UNorm_RG: format = GL_RG; break; // not sure about this one, need to check stbi
	case Texture::Format::UNorm_RGB:	format = GL_RGB; break;
	case Texture::Format::UNorm_RGBA: // fallthrough
	case Texture::Format::UNorm_BGRA: format = GL_RGBA; break;
	default: ASSERT(false);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 
		texture->GetWidth(), texture->GetHeight(), 
		0, format, GL_UNSIGNED_BYTE, texture->GetPixels());

	if (texture->IsUsingMipMaps())
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	return true;
}

void TextureGL::OnUnload(Graphics& aGraphics)
{
	ASSERT_STR(myGLTexture, "Attempt to free an uninitialized texture!");
	glDeleteTextures(1, &myGLTexture);
}

void TextureGL::UpdateTexParams(const Texture* aTexture)
{
	ASSERT_STR(myGLTexture, "Texture doesn't exist!");

	if (aTexture->GetWrapMode() != myWrapMode)
	{
		myWrapMode = aTexture->GetWrapMode();
		switch (myWrapMode)
		{
		case Texture::WrapMode::Repeat:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
		case Texture::WrapMode::Clamp:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
		case Texture::WrapMode::MirroredRepeat:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
		}
	}

	if (aTexture->GetMinFilter() != myMinFilter)
	{
		myMinFilter = aTexture->GetMinFilter();
		switch (myMinFilter)
		{
		case Texture::Filter::Nearest:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			break;
		case Texture::Filter::Linear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			break;
		case Texture::Filter::Nearest_MipMapNearest:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			break;
		case Texture::Filter::Linear_MipMapNearest:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			break;
		case Texture::Filter::Nearest_MipMapLinear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
			break;
		case Texture::Filter::Linear_MipMapLinear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			break;
		default:
			ASSERT(false);
		}
	}

	if (aTexture->GetMagFilter() != myMagFilter)
	{
		myMagFilter = aTexture->GetMagFilter();
		switch (myMagFilter)
		{
		case Texture::Filter::Nearest:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case Texture::Filter::Linear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		default:
			ASSERT(false);
		}
	}
}