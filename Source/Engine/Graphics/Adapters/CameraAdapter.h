#pragma once

#include <Graphics/UniformAdapterRegister.h>

class CameraAdapter : RegisterUniformAdapter<CameraAdapter>
{
public:
	constexpr static std::string_view kName = "CameraAdapter";
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};