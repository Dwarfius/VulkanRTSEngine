#pragma once

#include "Accessor.h"
#include <Graphics/Resources/Texture.h>

namespace glTF
{
	struct Image
	{
		enum class MimeType
		{
			None,
			Jpeg,
			PNG
		};

		std::string myUri;
		MimeType myMimeType;
		int myBufferView;
		std::string myName;

		static void ParseItem(const nlohmann::json& anImageJson, Image& anImage);
	};

	struct Sampler
	{
		::Texture::Filter myMagFilter;
		::Texture::Filter myMinFilter;
		::Texture::WrapMode myWrapU;
		::Texture::WrapMode myWrapV;
		std::string myName;

		static void ParseItem(const nlohmann::json& aSamplerJson, Sampler& aSampler);
	};

	struct Texture
	{
		int mySource;
		int mySampler;
		std::string myName;
		
		static void ParseItem(const nlohmann::json& aTextureJson, Texture& aTexture);

		struct TextureInputs : BufferAccessorInputs
		{
			const std::vector<Image>& myImages;
			const std::vector<Sampler>& mySamplers;
			const std::vector<Texture>& myTextures;
		};
		static void ConstructTextures(const TextureInputs& aInputs,
			std::vector<Handle<::Texture>>& aTextures,
			const std::string& aRelPath);
	};
}