#pragma once

#include <Core/DataEnum.h>

class ITexture
{
public:
	// TODO: split this into SourceFormat and DestFormat
	DATA_ENUM(Format, char,
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
	);

	DATA_ENUM(WrapMode, char,
		Clamp,
		Repeat,
		MirroredRepeat
	);

	DATA_ENUM(Filter, char,
		// Mag/Min
		Nearest,
		Linear,

		// Min only
		Nearest_MipMapNearest,
		Linear_MipMapNearest,
		Nearest_MipMapLinear,
		Linear_MipMapLinear
	);
};