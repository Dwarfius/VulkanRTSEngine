#pragma once

#include "Buffer.h"

namespace glTF
{
	struct BufferView
	{
		enum class Target
		{
			None,
			VertexBuffer,
			IndexBuffer
		};
		size_t myByteOffset;
		size_t myBufferLength;
		size_t myByteStride;
		uint32_t myBuffer;
		Target myTarget;

		template<class T>
		void ReadElem(T& anElem, size_t anIndex, size_t anAccessorOffset, const std::vector<Buffer>& aBuffers) const
		{
			const size_t stride = myByteStride > 0 ? myByteStride : sizeof(T);
			const char* elemPos = GetPtrToElem(stride, anIndex, anAccessorOffset, aBuffers);
			std::memcpy(&anElem, elemPos, sizeof(T));
		}

		const char* GetPtrToElem(size_t aStride, size_t anIndex, size_t anAccessorOffset, const std::vector<Buffer>& aBuffers) const
		{
			const Buffer& buffer = aBuffers[myBuffer];
			const char* bufferViewStart = buffer.data() + myByteOffset;
			return bufferViewStart + anAccessorOffset + anIndex * aStride;
		}

		static void ParseItem(const nlohmann::json& aViewJson, BufferView& aView)
		{
			aView.myBuffer = aViewJson["buffer"].get<uint32_t>();
			aView.myByteOffset = ReadOptional(aViewJson, "byteOffset", 0ull);
			aView.myBufferLength = aViewJson["byteLength"].get<size_t>();
			int target = ReadOptional(aViewJson, "target", kInvalidInd);
			switch (target)
			{
			case kInvalidInd: // TODO: [[fallthrough]]
				aView.myTarget = Target::None;
				break;
			case 34962: // GL_ARRAY_BUFFER
				aView.myTarget = Target::VertexBuffer;
				break;
			case 34963:
				aView.myTarget = Target::IndexBuffer;
				break;
			default:
				ASSERT_STR(false, "Unrecognized target! {}", target);
			}
			aView.myByteStride = ReadOptional(aViewJson, "byteStride", 0u);
		}
	};
}