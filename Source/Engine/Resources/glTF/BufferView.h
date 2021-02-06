#pragma once

#include "Buffer.h"

namespace glTF
{
	struct BufferView
	{
		enum class Target
		{
			None,
			VBO,
			EBO
		};
		size_t myByteOffset;
		size_t myBufferLength;
		size_t myByteStride;
		uint32_t myBuffer;
		uint32_t myTarget;

		template<class T>
		void ReadElem(T& anElem, size_t anIndex, size_t anAccessorOffset, const std::vector<Buffer>& aBuffers) const
		{
			const Buffer& buffer = aBuffers[myBuffer];
			const size_t stride = myByteStride > 0 ? myByteStride : sizeof(T);
			const char* bufferViewStart = buffer.data() + myByteOffset;
			const char* elemPos = bufferViewStart + anAccessorOffset + anIndex * stride;
			std::memcpy(&anElem, elemPos, sizeof(T));
		}

		static std::vector<BufferView> Parse(const nlohmann::json& aRootJson)
		{
			std::vector<BufferView> bufferViews;
			const nlohmann::json& bufferViewsJson = aRootJson["bufferViews"];
			bufferViews.reserve(bufferViewsJson.size());
			for (const nlohmann::json& bufferViewJson : bufferViewsJson)
			{
				BufferView view;
				view.myBuffer = bufferViewJson["buffer"].get<uint32_t>();
				view.myByteOffset = ReadOptional(bufferViewJson, "byteOffset", 0ull);
				view.myBufferLength = bufferViewJson["byteLength"].get<size_t>();
				// TODO: fix this target - use enum Target instead
				view.myTarget = ReadOptional(bufferViewJson, "target", 0u);
				view.myByteStride = ReadOptional(bufferViewJson, "byteStride", 0u);
				bufferViews.push_back(std::move(view));
			}
			return bufferViews;
		}
	};
}