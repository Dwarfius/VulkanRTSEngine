#include "Precomp.h"
#include "TextureGL.h"

#include <Graphics/Resources/Texture.h>
#include <Core/Profiler.h>

void TextureGL::BindTexture()
{
	ASSERT_STR(myGLTexture, "Missing GL texture to bind!");
	glBindTexture(GL_TEXTURE_2D, myGLTexture);
}

void TextureGL::BindImage(uint32_t aSlot, uint8_t aLevel, uint32_t anAccessType)
{
	ASSERT_STR(myGLTexture, "Missing GL texture to bind!");
	ASSERT_STR(anAccessType == GL_READ_ONLY, "Write NYI(last param must be write format)!");
	glBindImageTexture(aSlot, myGLTexture, aLevel, GL_FALSE, 0, anAccessType, TranslateInternalFormat(myFormat));
}

uint32_t TextureGL::TranslateInternalFormat(Format aFormat)
{
	switch (aFormat)
	{
	case Texture::Format::SNorm_R:		return GL_R8_SNORM;
	case Texture::Format::UNorm_R:		return GL_R8;
	case Texture::Format::I_R:			return GL_R32I;
	case Texture::Format::U_R:			return GL_R32UI;
	case Texture::Format::SNorm_RG:		return GL_RG8_SNORM;
	case Texture::Format::UNorm_RG:		return GL_RG8;
	case Texture::Format::I_RG:			return GL_RG32I;
	case Texture::Format::U_RG:			return GL_RG32UI;
	case Texture::Format::SNorm_RGB:	return GL_RGB8_SNORM;
	case Texture::Format::UNorm_RGB:	return GL_RGB8;
	case Texture::Format::I_RGB:		return GL_RGB32I;
	case Texture::Format::U_RGB:		return GL_RGB32UI;
	case Texture::Format::SNorm_RGBA:	return GL_RGBA8_SNORM;
	case Texture::Format::UNorm_RGBA:	return GL_RGBA8;
	case Texture::Format::UNorm_BGRA:	return GL_RGBA8;
	case Texture::Format::I_RGBA:		return GL_RGBA32I;
	case Texture::Format::U_RGBA:		return GL_RGBA32UI;
	case Texture::Format::Depth16:		return GL_DEPTH_COMPONENT16;
	case Texture::Format::Depth24:		return GL_DEPTH_COMPONENT24;
	case Texture::Format::Depth32F:		return GL_DEPTH_COMPONENT32F;
	case Texture::Format::Stencil8:		return GL_STENCIL_INDEX8;
	case Texture::Format::Depth24_Stencil8:		return GL_DEPTH24_STENCIL8;
	case Texture::Format::Depth32F_Stencil8:	return GL_DEPTH32F_STENCIL8;
	default: ASSERT(false);
	}
	static_assert(Format::GetSize() == 23, "Update above switch!");
	return 0;
}

uint32_t TextureGL::TranslateFormat(Format aFormat)
{
	switch (aFormat)
	{
	case Texture::Format::SNorm_R:
	case Texture::Format::UNorm_R:		return GL_RED;
	case Texture::Format::I_R:			
	case Texture::Format::U_R:			return GL_RED_INTEGER;
	case Texture::Format::SNorm_RG:
	case Texture::Format::UNorm_RG:		return GL_RG;
	case Texture::Format::I_RG:
	case Texture::Format::U_RG:			return GL_RG_INTEGER;
	case Texture::Format::SNorm_RGB:
	case Texture::Format::UNorm_RGB:	return GL_RGB;
	case Texture::Format::I_RGB:
	case Texture::Format::U_RGB:		return GL_RGB_INTEGER;
	case Texture::Format::SNorm_RGBA:
	case Texture::Format::UNorm_RGBA:	return GL_RGBA;
	case Texture::Format::UNorm_BGRA:	return GL_BGRA;
	case Texture::Format::I_RGBA:
	case Texture::Format::U_RGBA:		return GL_RGBA_INTEGER;
	case Texture::Format::Depth16:
	case Texture::Format::Depth24:
	case Texture::Format::Depth32F:		return GL_DEPTH_COMPONENT;
	case Texture::Format::Stencil8:		return GL_STENCIL_INDEX;
	case Texture::Format::Depth24_Stencil8:
	case Texture::Format::Depth32F_Stencil8:	return GL_DEPTH_STENCIL;
	default: ASSERT(false);
	}
	static_assert(Format::GetSize() == 23, "Update above switch!");
	return 0;
}

uint32_t TextureGL::DeterminePixelDataType(Format aFormat)
{
	switch (aFormat)
	{
	case Texture::Format::SNorm_R:
	case Texture::Format::SNorm_RG:
	case Texture::Format::SNorm_RGB:
	case Texture::Format::SNorm_RGBA:	return GL_BYTE;
	case Texture::Format::UNorm_R:		
	case Texture::Format::UNorm_RG:		
	case Texture::Format::UNorm_RGB:	
	case Texture::Format::UNorm_RGBA:	
	case Texture::Format::UNorm_BGRA:	return GL_UNSIGNED_BYTE;
	case Texture::Format::Depth16:		return GL_UNSIGNED_SHORT;
	case Texture::Format::I_R:
	case Texture::Format::I_RG:
	case Texture::Format::I_RGB:
	case Texture::Format::I_RGBA:		return GL_INT;
	case Texture::Format::U_R:
	case Texture::Format::U_RG:
	case Texture::Format::U_RGB:
	case Texture::Format::U_RGBA:
	case Texture::Format::Depth24:		return GL_UNSIGNED_INT;
	case Texture::Format::Depth32F:		return GL_FLOAT;
	case Texture::Format::Stencil8:		return GL_UNSIGNED_BYTE;
	case Texture::Format::Depth24_Stencil8:		return GL_UNSIGNED_INT_24_8;
	case Texture::Format::Depth32F_Stencil8:	return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
	default: ASSERT(false);
	}
	static_assert(Format::GetSize() == 23, "Update above switch!");
	return 0;
}

size_t TextureGL::GetPixelDataTypeSize(Format aFormat)
{
	switch (aFormat)
	{
	case Texture::Format::SNorm_R:
	case Texture::Format::UNorm_R:		return sizeof(char);
	case Texture::Format::SNorm_RG:
	case Texture::Format::UNorm_RG:		return sizeof(char) * 2;
	case Texture::Format::SNorm_RGB:
	case Texture::Format::UNorm_RGB:	return sizeof(char) * 3;
	case Texture::Format::SNorm_RGBA:
	case Texture::Format::UNorm_RGBA:
	case Texture::Format::UNorm_BGRA:	return sizeof(char) * 4;
	case Texture::Format::I_R:
	case Texture::Format::U_R:			return sizeof(int);
	case Texture::Format::I_RG:
	case Texture::Format::U_RG:			return sizeof(int) * 2;
	case Texture::Format::I_RGB:
	case Texture::Format::U_RGB:		return sizeof(int) * 3;
	case Texture::Format::I_RGBA:
	case Texture::Format::U_RGBA:		return sizeof(int) * 4;
	case Texture::Format::Depth16:		return sizeof(char) * 2;
	case Texture::Format::Depth24:		return sizeof(char) * 3;
	case Texture::Format::Depth32F:		return sizeof(char) * 4;
	case Texture::Format::Stencil8:		return sizeof(char);
	case Texture::Format::Depth24_Stencil8: return sizeof(char) * 4;
	case Texture::Format::Depth32F_Stencil8:	return sizeof(char) * 5;
	default: ASSERT(false);
	}
	static_assert(Format::GetSize() == 23, "Update above switch!");
	return 0;
}

void TextureGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myGLTexture, "Recreating an existing texture!");
	glGenTextures(1, &myGLTexture);

	const Texture* texture = myResHandle.Get<const Texture>();
	mySize = glm::vec2(texture->GetWidth(), texture->GetHeight());
}

bool TextureGL::OnUpload(Graphics& aGraphics)
{
	Profiler::ScopedMark uploadMark("TextureGL::OnUpload");

	ASSERT_STR(myGLTexture, "Uploading an uploaded texture!");

	// rebind it in case it's not bound
	glBindTexture(GL_TEXTURE_2D, myGLTexture);

	const Texture* texture = myResHandle.Get<const Texture>();
	UpdateTexParams(texture);

	const GLenum format = TranslateFormat(texture->GetFormat());
	const GLint internFormat = TranslateInternalFormat(texture->GetFormat());
	const GLenum pixelType = DeterminePixelDataType(texture->GetFormat());

	// TODO: hook into internal format preferences
	// glGetInternalFormativ(GL_TEXTURE_2D, GL_RGBA8, GL_TEXTURE_IMAGE_FORMAT, 1, &preferred_format)

	glTexImage2D(GL_TEXTURE_2D, 0, internFormat,
		texture->GetWidth(), texture->GetHeight(), 
		0, format, pixelType, texture->GetPixels());

	if (texture->IsUsingMipMaps())
	{
		Profiler::ScopedMark mipmapsMark("TextureGL::GenerateMipMaps");
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

	myFormat = aTexture->GetFormat();

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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			break;
		case Texture::WrapMode::MirroredRepeat:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			break;
		default:
			ASSERT(false);
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