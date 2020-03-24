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
		kLastSignedFormat = SNorm_RGBA,

		UNorm_R,
		UNorm_RG,
		UNorm_RGB,
		UNorm_RGBA,

		// Special
		UNorm_BGRA // VK relies on this for swapchain initialization
	};

	enum class WrapMode
	{
		Clamp,
		Repeat,
		MirroredRepeat
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
};