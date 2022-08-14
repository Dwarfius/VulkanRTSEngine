#pragma once

#include <Core/DataEnum.h>

class ITexture
{
public:
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

		// 32-byte per component
		I_R,
		I_RG,
		I_RGB,
		I_RGBA,

		U_R,
		U_RG,
		U_RGB,
		U_RGBA,

		// Depth
		Depth16,
		Depth24,
		Depth32F,

		// Stencil
		Stencil8,

		// Merged Depth+Stencil
		Depth24_Stencil8,
		Depth32F_Stencil8,

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