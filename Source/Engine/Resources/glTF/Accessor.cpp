#pragma once
#include "Precomp.h"
#include "Accessor.h"

namespace glTF
{
	void Accessor::Sparse::ParseItem(const nlohmann::json& aSparseJson, Sparse& aSparse)
	{
		aSparse.myCount = aSparseJson["count"].get<size_t>();
		ASSERT_STR(aSparse.myCount > 0, "According to spec it must be at least 1!");

		const nlohmann::json& indicesJson = aSparseJson["indices"];
		aSparse.myIndices.myBufferView = indicesJson["bufferView"].get<uint32_t>();
		aSparse.myIndices.myByteOffset = ReadOptional(indicesJson, "byteOffset", 0ull);
		uint32_t componentType = indicesJson["componentType"].get<uint32_t>();
		aSparse.myIndices.myComponentType = Accessor::IdentifyCompType(componentType);
		
		const nlohmann::json& valuesJson = aSparseJson["values"];
		aSparse.myValues.myBufferView = valuesJson["bufferView"].get<uint32_t>();
		aSparse.myValues.myByteOffset = ReadOptional(valuesJson, "byteOffset", 0ull);
	}

	void Accessor::ParseItem(const nlohmann::json& anAccessortJson, Accessor& anAccessor)
	{
		anAccessor.myBufferView = ReadOptional(anAccessortJson, "bufferView", 0u);
		anAccessor.myByteOffset = ReadOptional(anAccessortJson, "byteOffset", 0ull);
		anAccessor.myCount = anAccessortJson["count"].get<size_t>();
		anAccessor.myIsNormalized = ReadOptional(anAccessortJson, "normalized", false);

		auto sparseJsonIter = anAccessortJson.find("sparse");
		if (sparseJsonIter != anAccessortJson.end())
		{
			Sparse::ParseItem(*sparseJsonIter, anAccessor.mySparse);
		}

		std::string typeStr = anAccessortJson["type"].get<std::string>();
		anAccessor.myType = Accessor::Type::Scalar;
		if (typeStr == "SCALAR")
		{
			anAccessor.myType = Accessor::Type::Scalar;
		}
		else if (typeStr == "VEC2")
		{
			anAccessor.myType = Accessor::Type::Vec2;
		}
		else if (typeStr == "VEC3")
		{
			anAccessor.myType = Accessor::Type::Vec3;
		}
		else if (typeStr == "VEC4")
		{
			anAccessor.myType = Accessor::Type::Vec4;
		}
		else if (typeStr == "MAT2")
		{
			anAccessor.myType = Accessor::Type::Mat2;
		}
		else if (typeStr == "MAT3")
		{
			anAccessor.myType = Accessor::Type::Mat3;
		}
		else if (typeStr == "MAT4")
		{
			anAccessor.myType = Accessor::Type::Mat4;
		}
		else
		{
			ASSERT(false);
		}

		uint32_t componentType = anAccessortJson["componentType"].get<uint32_t>();
		anAccessor.myComponentType = IdentifyCompType(componentType);

		const auto& maxIter = anAccessortJson.find("max");
		if (maxIter != anAccessortJson.end())
		{
			ExtractLimit(*maxIter, anAccessor.myMax, anAccessor.myType, anAccessor.myComponentType);
		}
		const auto& minIter = anAccessortJson.find("min");
		if (minIter != anAccessortJson.end())
		{
			ExtractLimit(*minIter, anAccessor.myMin, anAccessor.myType, anAccessor.myComponentType);
		}
	}

	void Accessor::ExtractLimit(const nlohmann::json& aJson,
		uint32_t(&aMember)[16],
		Accessor::Type aType,
		Accessor::ComponentType aCompType)
	{
		auto Read = [](const nlohmann::json& aJson,
			uint32_t& anElem,
			Accessor::ComponentType aCompType)
		{
			switch (aCompType)
			{
			case Accessor::ComponentType::Byte:
			{
				const int8_t data = aJson.get<int8_t>();
				std::memcpy(&anElem, &data, sizeof(int8_t));
				break;
			}
			case Accessor::ComponentType::UnsignedByte:
			{
				const uint8_t data = aJson.get<uint8_t>();
				std::memcpy(&anElem, &data, sizeof(uint8_t));
				break;
			}
			case Accessor::ComponentType::Short:
			{
				const int16_t data = aJson.get<int16_t>();
				std::memcpy(&anElem, &data, sizeof(int16_t));
				break;
			}
			case Accessor::ComponentType::UnsignedShort:
			{
				const uint16_t data = aJson.get<uint16_t>();
				std::memcpy(&anElem, &data, sizeof(uint16_t));
				break;
			}
			case Accessor::ComponentType::UnsignedInt:
			{
				const uint32_t data = aJson.get<uint32_t>();
				std::memcpy(&anElem, &data, sizeof(uint32_t));
				break;
			}
			case Accessor::ComponentType::Float:
			{
				const float data = aJson.get<float>();
				std::memcpy(&anElem, &data, sizeof(float));
				break;
			}
			default: ASSERT(false);
			}
		};

		uint8_t iters = GetElemCount(aType);
		ASSERT(iters != 0);
		for (uint8_t i = 0; i < iters; i++)
		{
			Read(aJson.at(i), aMember[i], aCompType);
		}
	}

	bool Accessor::HasSparseBuffer() const
	{
		return mySparse.myCount != 0;
	}

	void Accessor::ReconstructSparseBuffer(std::vector<BufferView>& aBufferViews, std::vector<Buffer>& aBuffers)
	{
		ASSERT_STR(HasSparseBuffer(), "Invalid call, not a sparse accessor!");
		// Reconstructing buffers is very memory wasteful, but it's either this
		// or slowing down the buffer reads considerably
		const BufferView& origView = aBufferViews[myBufferView];
		const Buffer& origBuffer = aBuffers[origView.myBuffer];

		// backing in the offsets to reduce the new buffer size
		size_t bufferStart = myByteOffset + origView.myByteOffset;
		size_t bufferLength = origView.myBufferLength - myByteOffset;

		Buffer newBuffer;
		newBuffer.resize(bufferLength);
		std::memcpy(newBuffer.data(), origBuffer.data() + bufferStart, bufferLength);

		// now filling in the sparse data
		const BufferView& indicesView = aBufferViews[mySparse.myIndices.myBufferView];
		const BufferView& dataView = aBufferViews[mySparse.myValues.myBufferView];
		const size_t elemSize = GetElemSize(myComponentType, myType);
		for (size_t i = 0; i < mySparse.myCount; i++)
		{
			uint32_t index;
			switch (mySparse.myIndices.myComponentType)
			{
			case ComponentType::UnsignedByte:
			{
				uint8_t temp;
				indicesView.ReadElem(temp, i, mySparse.myIndices.myByteOffset, aBuffers);
				index = temp;
				break;
			}
			case ComponentType::UnsignedShort:
			{
				uint16_t temp;
				indicesView.ReadElem(temp, i, mySparse.myIndices.myByteOffset, aBuffers);
				index = temp;
				break;
			}
			case ComponentType::UnsignedInt:
				indicesView.ReadElem(index, i, mySparse.myIndices.myByteOffset, aBuffers);
				break;
			default:
				ASSERT_STR(false, "Unsupported component type!");
			}

			const char* newElemPtr = dataView.GetPtrToElem(elemSize, i, mySparse.myValues.myByteOffset, aBuffers);
			std::memcpy(newBuffer.data() + elemSize * index, newElemPtr, elemSize);
		}

		BufferView newView = origView;
		newView.myByteOffset = 0; // baked in above
		newView.myBuffer = aBuffers.size();

		// repoint to new view
		myBufferView = aBufferViews.size();
		myByteOffset = 0; // baked in above

		aBufferViews.push_back(std::move(newView));
		aBuffers.push_back(std::move(newBuffer));
	}

	constexpr Accessor::ComponentType Accessor::IdentifyCompType(uint32_t anId)
	{
		switch (anId)
		{
		case 5120: // GL_BYTE
			return Accessor::ComponentType::Byte;
		case 5121: // GL_UNSIGNED_BYTE
			return Accessor::ComponentType::UnsignedByte;
		case 5122: // GL_SHORT
			return Accessor::ComponentType::Short;
		case 5123: // GL_UNSIGNED_SHORT
			return Accessor::ComponentType::UnsignedShort;
		case 5125: // GL_UNSIGNED_INT
			return Accessor::ComponentType::UnsignedInt;
		case 5126: // GL_FLOAT
			return Accessor::ComponentType::Float;
		default: 
			ASSERT_STR(false, "Invalid id passed in - unrecognized Component Type!");
			return Accessor::ComponentType::Byte;
		}
	}
}