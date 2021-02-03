#pragma once

#include "BufferView.h"

namespace glTF
{
	struct Accessor
	{
		enum class ComponentType : unsigned char
		{
			Byte,
			UnsignedByte,
			Short,
			UnsignedShort,
			UnsignedInt,
			Float
		};

		enum class Type : unsigned char
		{
			Scalar,
			Vec2,
			Vec3,
			Vec4,
			Mat2,
			Mat3,
			Mat4
		};
		constexpr static uint8_t GetElemCount(Type aType)
		{
			uint8_t count = 0;
			switch (aType)
			{
			case Type::Scalar: count = 1; break;
			case Type::Vec2: count = 2; break;
			case Type::Vec3: count = 3; break;
			case Type::Vec4: count = 4; break;
			case Type::Mat2: count = 4; break;
			case Type::Mat3: count = 9; break;
			case Type::Mat4: count = 16; break;
			}
			return count;
		}

		constexpr static size_t GetElemSize(ComponentType aCompType, Type aType)
		{
			uint8_t count = GetElemCount(aType);
			size_t size = 0;
			switch (aCompType)
			{
			case ComponentType::Byte: size = 1; break;
			case ComponentType::UnsignedByte: size = 1; break;
			case ComponentType::Short: size = 2; break;
			case ComponentType::UnsignedShort: size = 2; break;
			case ComponentType::UnsignedInt: size = 4; break;
			case ComponentType::Float: size = 4; break;
			}
			return count * size;
		}

		size_t myByteOffset;
		size_t myCount;
		uint32_t myMax[16];
		uint32_t myMin[16];
		uint32_t myBufferView;
		ComponentType myComponentType;
		Type myType;
		bool myIsNormalized;

		template<class T>
		void ReadElem(T& anElem, size_t anIndex, const std::vector<BufferView>& aViews, const std::vector<Buffer>& aBuffers) const
		{
			ASSERT_STR(sizeof(T) == GetElemSize(myComponentType, myType),
				"Missmatched T passed in, the resulting element will not be properly read!");
				
			const BufferView& view = aViews[myBufferView];
			view.ReadElem(anElem, anIndex, myByteOffset, aBuffers);
		}

		static std::vector<Accessor> Parse(const nlohmann::json& aRootJson)
		{
			auto ExtractLimit = [](const nlohmann::json& aJson,
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
			};

			std::vector<Accessor> accessors;
			const nlohmann::json& accessorsJson = aRootJson["accessors"];
			accessors.reserve(accessorsJson.size());
			for (const nlohmann::json& accessorJson : accessorsJson)
			{
				{
					auto sparseJsonIter = accessorJson.find("sparse");
					ASSERT_STR(sparseJsonIter == accessorJson.end(), "Sparse not yet implemented!");
				}

				Accessor accessor;
				// TODO: support optional bufferView!
				accessor.myBufferView = accessorJson["bufferView"].get<uint32_t>();
				accessor.myByteOffset = ReadOptional(accessorJson, "byteOffset", 0ull);
				accessor.myCount = accessorJson["count"].get<size_t>();
				accessor.myIsNormalized = ReadOptional(accessorJson, "normalized", false);

				std::string typeStr = accessorJson["type"].get<std::string>();
				accessor.myType = Accessor::Type::Scalar;
				if (typeStr == "SCALAR")
				{
					accessor.myType = Accessor::Type::Scalar;
				}
				else if (typeStr == "VEC2")
				{
					accessor.myType = Accessor::Type::Vec2;
				}
				else if (typeStr == "VEC3")
				{
					accessor.myType = Accessor::Type::Vec3;
				}
				else if (typeStr == "VEC4")
				{
					accessor.myType = Accessor::Type::Vec4;
				}
				else if (typeStr == "MAT2")
				{
					accessor.myType = Accessor::Type::Mat2;
				}
				else if (typeStr == "MAT3")
				{
					accessor.myType = Accessor::Type::Mat3;
				}
				else if (typeStr == "MAT4")
				{
					accessor.myType = Accessor::Type::Mat4;
				}
				else
				{
					ASSERT(false);
				}

				Accessor::ComponentType compType = Accessor::ComponentType::Byte;
				uint32_t componentType = accessorJson["componentType"].get<uint32_t>();
				switch (componentType)
				{
				case GL_BYTE: compType = Accessor::ComponentType::Byte; break;
				case GL_UNSIGNED_BYTE: compType = Accessor::ComponentType::UnsignedByte; break;
				case GL_SHORT: compType = Accessor::ComponentType::Short; break;
				case GL_UNSIGNED_SHORT: compType = Accessor::ComponentType::UnsignedShort; break;
				case GL_UNSIGNED_INT: compType = Accessor::ComponentType::UnsignedInt; break;
				case GL_FLOAT: compType = Accessor::ComponentType::Float; break;
				default: ASSERT(false);
				}
				accessor.myComponentType = compType;

				const auto& maxIter = accessorJson.find("max");
				if (maxIter != accessorJson.end())
				{
					ExtractLimit(*maxIter, accessor.myMax, accessor.myType, compType);
				}
				const auto& minIter = accessorJson.find("min");
				if (minIter != accessorJson.end())
				{
					ExtractLimit(*minIter, accessor.myMin, accessor.myType, compType);
				}

				accessors.push_back(std::move(accessor));
			}
			return accessors;
		}
	};

	// The default pack of inputs to accesss data stored in buffers
	struct BufferAccessorInputs
	{
		const std::vector<Buffer>& myBuffers;
		const std::vector<BufferView>& myBufferViews;
		const std::vector<Accessor>& myAccessors;
	};
}