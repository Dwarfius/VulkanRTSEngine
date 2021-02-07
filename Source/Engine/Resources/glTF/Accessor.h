#pragma once

#include "BufferView.h"

namespace glTF
{
	struct Accessor;
	// The default pack of inputs to accesss data stored in buffers
	struct BufferAccessorInputs
	{
		const std::vector<Buffer>& myBuffers;
		const std::vector<BufferView>& myBufferViews;
		const std::vector<Accessor>& myAccessors;
	};

	struct Accessor
	{
	private:
		// Util to detemine size of arrays and compound glm types
		template<class T>
		constexpr static size_t ArraySize = std::is_array_v<T> ? std::extent_v<T> : // array
			// glm mat4 special case - length is of a row not entire mat4!
			std::is_same_v<T, glm::mat4> ? 16 :
			// simple case of single arithmetic variable
			std::is_arithmetic_v<T> ? 1 :
			// must be something custom - 
			// try to fall back to glm's static length method
			T::length();

		template<class T, size_t TElems>
		struct ReadAndDenormalizeJunc
		{
			static void Call(T& anElem, size_t anIndex,
				const std::vector<BufferView>& aViews,
				const std::vector<Buffer>& aBuffers,
				const Accessor& anAccessor);
		};

		template<class T>
		struct ReadAndDenormalizeJunc<T, 1>
		{
			static void Call(T& anElem, size_t anIndex,
				const std::vector<BufferView>& aViews,
				const std::vector<Buffer>& aBuffers,
				const Accessor& anAccessor);
		};

	public:
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

		struct Sparse
		{
			struct Indices
			{
				size_t myByteOffset = 0;
				uint32_t myBufferView; // req
				ComponentType myComponentType; // req
			};

			struct Values
			{
				size_t myByteOffset = 0;
				uint32_t myBufferView; // req
			};

			Indices myIndices; // req
			Values myValues; // req
			size_t myCount = 0; // req, but we use this to signal if it's set or not

			static void ParseItem(const nlohmann::json& aSparseJson, Sparse& aSparse);
		};
		
		size_t myByteOffset = 0;
		size_t myCount = 0; // req
		uint32_t myMax[16] = {};
		uint32_t myMin[16] = {};
		Sparse mySparse;
		uint32_t myBufferView = 0;
		ComponentType myComponentType; // req
		Type myType; // req
		bool myIsNormalized = false;

		static void ParseItem(const nlohmann::json& anAccessortJson, Accessor& anAccessor);

		bool HasSparseBuffer() const;
		void ReconstructSparseBuffer(std::vector<BufferView>& aBufferViews, std::vector<Buffer>& aBuffers);

		template<class T>
		void ReadElem(T& anElem, size_t anIndex, const std::vector<BufferView>& aViews, const std::vector<Buffer>& aBuffers) const
		{
			ASSERT_STR(sizeof(T) == GetElemSize(myComponentType, myType),
				"Missmatched T passed in, the resulting element will not be properly read!");
				
			const BufferView& view = aViews[myBufferView];
			view.ReadElem(anElem, anIndex, myByteOffset, aBuffers);
		}

		// Reads an element as gltf declared type and denormalizes to passed in T type
		// Supports float, u8, u16
		template<class T, size_t TElems = ArraySize<T>>
		void ReadAndDenormalize(T& anElem, size_t anIndex,
			const std::vector<BufferView>& aViews,
			const std::vector<Buffer>& aBuffers) const
		{
			ReadAndDenormalizeJunc<T, TElems>::Call(anElem, anIndex, aViews, aBuffers, *this);
		}

	private:
		constexpr static ComponentType IdentifyCompType(uint32_t anId);

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

		static void ExtractLimit(const nlohmann::json& aJson,
			uint32_t(&aMember)[16],
			Accessor::Type aType,
			Accessor::ComponentType aCompType);
	};

	template<class T, size_t TElems>
	void Accessor::ReadAndDenormalizeJunc<T, TElems>::Call(T& anElem, size_t anIndex,
		const std::vector<BufferView>& aViews,
		const std::vector<Buffer>& aBuffers,
		const Accessor& anAccessor)
	{
		ComponentType compType = anAccessor.myComponentType;
		Type type = anAccessor.myType;
		ASSERT_STR(sizeof(T) == GetElemSize(compType, type),
			"Missmatched T passed in, the resulting element will not be properly read!");

		ASSERT_STR(GetElemCount(type) > 1, "Doesn't handle single elements!");
		switch (compType)
		{
		case ComponentType::Float:
			// no denorm needed!
			anAccessor.ReadElem(anElem, anIndex, aViews, aBuffers);
			break;
		case ComponentType::UnsignedByte:
		{
			ASSERT_STR(anAccessor.myIsNormalized, "Must be normalized according to doc!");
			uint8_t temp[TElems];
			anAccessor.ReadElem(temp, anIndex, aViews, aBuffers);
			for (uint8_t elemIndex = 0; elemIndex < TElems; elemIndex++)
			{
				anElem[elemIndex] = temp[elemIndex] / static_cast<float>(std::numeric_limits<uint8_t>::max());
			}
			break;
		}
		case ComponentType::UnsignedShort:
		{
			ASSERT_STR(anAccessor.myIsNormalized, "Must be normalized according to doc!");
			uint16_t temp[TElems];
			anAccessor.ReadElem(temp, anIndex, aViews, aBuffers);
			for (uint8_t elemIndex = 0; elemIndex < TElems; elemIndex++)
			{
				anElem[elemIndex] = temp[elemIndex] / static_cast<float>(std::numeric_limits<uint16_t>::max());;
			}
			break;
		}
		default:
			ASSERT_STR(false, "Unsupported component type!");
		}
	}

	template<class T>
	void Accessor::ReadAndDenormalizeJunc<T, 1>::Call(T& anElem, size_t anIndex,
		const std::vector<BufferView>& aViews,
		const std::vector<Buffer>& aBuffers,
		const Accessor& anAccessor)
	{
		ComponentType compType = anAccessor.myComponentType;
		Type type = anAccessor.myType;
		ASSERT_STR(sizeof(T) == GetElemSize(compType, type),
			"Missmatched T passed in, the resulting element will not be properly read!");

		// This is brittle, but should suffice
		ASSERT_STR(GetElemCount(type) == 1, "Doesn't handle multiple elements!");
		switch (compType)
		{
		case ComponentType::Float:
			// no denorm needed!
			anAccessor.ReadElem(anElem, anIndex, aViews, aBuffers);
			break;
		case ComponentType::UnsignedByte:
		{
			ASSERT_STR(anAccessor.myIsNormalized, "Must be normalized according to doc!");
			uint8_t temp;
			anAccessor.ReadElem(temp, anIndex, aViews, aBuffers);
			anElem = temp / static_cast<float>(std::numeric_limits<uint8_t>::max());
			break;
		}
		case ComponentType::UnsignedShort:
		{
			ASSERT_STR(anAccessor.myIsNormalized, "Must be normalized according to doc!");
			uint16_t temp;
			anAccessor.ReadElem(temp, anIndex, aViews, aBuffers);
			anElem = temp / static_cast<float>(std::numeric_limits<uint16_t>::max());;
			break;
		}
		default:
			ASSERT_STR(false, "Unsupported component type!");
		}
	}
}