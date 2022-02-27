#pragma once

#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Descriptor.h>

class CameraAdapter : RegisterUniformAdapter<CameraAdapter>
{
public:
	constexpr static std::string_view kName = "CameraAdapter";
	inline static const Descriptor ourDescriptor{
		{ Descriptor::UniformType::Mat4 },
		{ Descriptor::UniformType::Mat4 },
		{ Descriptor::UniformType::Mat4 },
		{ Descriptor::UniformType::Vec4, 6 },
		{ Descriptor::UniformType::Vec4 },
		{ Descriptor::UniformType::Vec2 }
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};