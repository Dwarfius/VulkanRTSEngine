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

		static std::vector<Image> Parse(const nlohmann::json& aRoot);
	};

	struct Sampler
	{
		::Texture::Filter myMagFilter;
		::Texture::Filter myMinFilter;
		::Texture::WrapMode myWrapU;
		::Texture::WrapMode myWrapV;
		std::string myName;

		static std::vector<Sampler> Parse(const nlohmann::json& aRoot);
	};

	struct Texture
	{
		int mySource;
		int mySampler;
		std::string myName;
		
		static std::vector<Texture> Parse(const nlohmann::json& aRoot);

		struct TextureInputs : BufferAccessorInputs
		{
			const std::vector<Image>& myImages;
			const std::vector<Sampler>& mySamplers;
			const std::vector<Texture>& myTextures;
		};
		static void ConstructTextures(const TextureInputs& aInputs,
			std::vector<Handle<::Texture>>& aTextures);
	};
}