#pragma once

class ITexture
{
public:
	// TODO: split this into SourceFormat and DestFormat
	enum class Format
	{
		// Generic
		// 8 byte per component
		SNorm_R,
		SNorm_RG,
		SNorm_RGB,
		SNorm_RGBA,

		UNorm_R,
		UNorm_RG,
		UNorm_RGB,
		UNorm_RGBA,

		// Special
		UNorm_BGRA // VK relies on this for swapchain initialization
	};
	constexpr static const char* const kFormatNames[]
	{
		"SNorm_R",
		"SNorm_RG",
		"SNorm_RGB",
		"SNorm_RGBA",
		
		"UNorm_R",
		"UNorm_RG",
		"UNorm_RGB",
		"UNorm_RGBA",

		"UNorm_BGRA" 
	};

	enum class WrapMode
	{
		Clamp,
		Repeat,
		MirroredRepeat
	};
	constexpr static const char* const kWrapModeNames[]
	{
		"Clamp",
		"Repeat",
		"MirroredRepeat"
	};

	enum class Filter
	{
		// Mag/Min
		Nearest,
		Linear,

		// Min only
		Nearest_MipMapNearest,
		Linear_MipMapNearest,
		Nearest_MipMapLinear,
		Linear_MipMapLinear
	};
	constexpr static const char* const kFilterNames[]
	{
		"Nearest",
		"Linear",

		"Nearest_MipMapNearest",
		"Linear_MipMapNearest",
		"Nearest_MipMapLinear",
		"Linear_MipMapLinear"
	};
};