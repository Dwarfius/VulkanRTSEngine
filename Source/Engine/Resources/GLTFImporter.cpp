#include "Precomp.h"
#include "GLTFImporter.h"

#include <Graphics/Resources/Model.h>
#include <Core/File.h>
#include <Core/Utils.h>
#include <Core/Vertex.h>
#include <nlohmann/json.hpp>
#include <charconv>
#include <system_error>

namespace GLTFImpl_detail
{
	template<class T>
	T ReadOptional(const nlohmann::json& aJson, std::string_view aKey, T aDefaultVal)
	{
		const auto& iter = aJson.find(aKey);
		if (iter != aJson.end())
		{
			return iter->get<T>();
		}
		return aDefaultVal;
	}

	bool FindVersion(const nlohmann::json& aJson, std::string& aVersion)
	{
		const auto& assetJson = aJson.find("asset");
		if (assetJson == aJson.end())
		{
			return false;
		}

		const auto& versionJson = assetJson->find("version");
		if (versionJson == assetJson->end())
		{
			return false;
		}

		aVersion = versionJson->get<std::string>();
		return true;
	}

	using Buffer = std::vector<char>;
	std::vector<Buffer> ReadBuffers(const nlohmann::json& aRootJson)
	{
		std::vector<Buffer> buffers;
		const nlohmann::json& buffersJson = aRootJson["buffers"];
		buffers.reserve(buffersJson.size());
		for (const nlohmann::json& bufferJson : buffersJson)
		{
			Buffer buffer;
			std::string uri = bufferJson["uri"].get<std::string>();
			std::string_view uriPrefix(uri.c_str(), 5);
			if (uriPrefix == "data:")
			{
				size_t separatorInd = uri.find(';', 5);
				std::string_view mimeType(uri.c_str() + 5, separatorInd - 5);
				
				if (mimeType != "application/octet-stream")
				{
					ASSERT_STR(false, "Only Octet-stream data uri is supported!");
					continue;
				}
				
				std::string_view encoding(uri.c_str() + separatorInd + 1, 6);
				if (encoding != "base64")
				{
					ASSERT_STR(false, "Only base64 encoding of data uri is supported!");
					continue;
				}

				std::string_view data(uri.c_str() + separatorInd + 8);
				buffer = Utils::Base64Decode(data);
			}
			else
			{
				// it's a file, and we assume it's located nearby
				File file(uri);
				if (!file.Read())
				{
					ASSERT_STR(false, "Failed to read %s", file.GetPath());
					continue;
				}
				// TODO: this is a copy, after File's TODO is done,
				// change to use buffer stealing
				const std::string& fileBuff = file.GetBuffer();
				buffer.assign(fileBuff.begin(), fileBuff.end());
			}

			size_t byteLength = bufferJson["byteLength"].get<size_t>();
			ASSERT(buffer.size() == byteLength);
			buffers.emplace_back(std::move(buffer));
		}
		return buffers;
	}

	// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0/#reference-bufferview
	struct BufferView
	{
		size_t myByteOffset;
		size_t myBufferLength;
		size_t myByteStride;
		uint32_t myBuffer;
		uint32_t myTarget; // GL constant for buffer type

		template<class T>
		void ReadElem(T& anElem, uint32_t anIndex, size_t anAccessorOffset, const std::vector<Buffer>& aBuffers) const
		{
			const Buffer& buffer = aBuffers[myBuffer];
			const size_t stride = myByteStride > 0 ? myByteStride : sizeof(T);
			const char* bufferViewStart = buffer.data() + myByteOffset;
			const char* elemPos = bufferViewStart + anAccessorOffset + anIndex * stride;
			std::memcpy(&anElem, elemPos, sizeof(T));
		}
	};

	std::vector<BufferView> ReadBufferViews(const nlohmann::json& aRootJson)
	{
		std::vector<BufferView> bufferViews;
		const nlohmann::json& bufferViewsJson = aRootJson["bufferViews"];
		bufferViews.reserve(bufferViewsJson.size());
		for (const nlohmann::json& bufferViewJson : bufferViewsJson)
		{
			uint32_t buffer = bufferViewJson["buffer"].get<uint32_t>();
			size_t byteOffset = ReadOptional(bufferViewJson, "byteOffset", 0ull);
			size_t byteLength = bufferViewJson["byteLength"].get<size_t>();
			uint32_t target = ReadOptional(bufferViewJson, "target", 0u);
			uint32_t byteStride = ReadOptional(bufferViewJson, "byteStride", 0u);
			bufferViews.push_back({ byteOffset, byteLength, byteStride, buffer, target });
		}
		return bufferViews;
	}

	// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0/#reference-accessor
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

		size_t myByteOffset;
		size_t myCount;
		uint32_t myMax[16];
		uint32_t myMin[16];
		uint32_t myBufferView;
		ComponentType myComponentType;
		Type myType;
		bool myIsNormalized;

		template<class T>
		void ReadElem(T& anElem, uint32_t anIndex, const std::vector<BufferView>& aViews, const std::vector<Buffer>& aBuffers) const
		{
			const BufferView& view = aViews[myBufferView];
			view.ReadElem(anElem, anIndex, myByteOffset, aBuffers);
		}
	};

	std::vector<Accessor> ReadAccessors(const nlohmann::json& aRootJson)
	{
		auto ExtractLimit = [](const nlohmann::json& aJson, 
			uint32_t (&aMember)[16],
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
				default: ASSERT(false); break;
				}
			};

			uint8_t iters = 0;
			switch (aType)
			{
			case Accessor::Type::Scalar: iters = 1; break;
			case Accessor::Type::Vec2: iters = 2; break;
			case Accessor::Type::Vec3: iters = 3; break;
			case Accessor::Type::Vec4: iters = 4; break;
			case Accessor::Type::Mat2: iters = 4; break;
			case Accessor::Type::Mat3: iters = 9; break;
			case Accessor::Type::Mat4: iters = 16; break;
			default: ASSERT(false); break;
			}

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
			default: ASSERT(false); break;
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

			accessors.push_back(accessor);
		}
		return accessors;
	}

	struct Attribute
	{
		std::string myType;
		uint32_t myAccessor;
	};
	struct Mesh
	{
		std::vector<Attribute> myAttributes;
		uint32_t myIndexAccessor : 31;
		uint32_t myHasIndices : 1;
	};
	std::vector<Mesh> ReadMeshes(const nlohmann::json& aRootJson)
	{
		std::vector<Mesh> meshes;
		const nlohmann::json& meshesJson = aRootJson["meshes"];
		for (const nlohmann::json& meshJson : meshesJson)
		{
			Mesh mesh;
			const nlohmann::json& primitivesJson = meshJson["primitives"];
			ASSERT_STR(primitivesJson.size() == 1, "At the moment we're assuming"
				" 1 primitive set per mesh!");
			for (const nlohmann::json& primitiveJson : primitivesJson)
			{
				const nlohmann::json& attributesJson = primitiveJson["attributes"];
				for (auto attribIter = attributesJson.begin();
					attribIter != attributesJson.end();
					attribIter++)
				{
					Attribute attribute;
					attribute.myType = attribIter.key();
					attribute.myAccessor = attribIter->get<uint32_t>();
					mesh.myAttributes.emplace_back(std::move(attribute));
				}

				const auto& indicesJsonIter = primitiveJson.find("indices");
				if (indicesJsonIter != primitiveJson.end())
				{
					mesh.myHasIndices = true;
					mesh.myIndexAccessor = indicesJsonIter->get<uint32_t>();
				}
				else
				{
					mesh.myHasIndices = false;
				}
			}
			meshes.push_back(mesh);
		}
		return meshes;
	}
}

GLTFImporter::GLTFImporter(Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myModel(new Model(PrimitiveType::Triangles, nullptr, true))
{
}

void GLTFImporter::OnLoad(const File& aFile)
{
	using namespace GLTFImpl_detail;
	// time to start learning glTF!
	// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_002_BasicGltfStructure.md
	nlohmann::json gltfJson = nlohmann::json::parse(aFile.GetBuffer(), nullptr, false);
	if (!gltfJson.is_object())
	{
		SetErrMsg("Failed to parse file!");
		return;
	}

	{
		std::string version;
		if (!FindVersion(gltfJson, version))
		{
			SetErrMsg("Failed to find asset version, unsupported version!");
			return;
		}

		if (version.compare("2.0"))
		{
			SetErrMsg("Found asset version unsupported: " + version);
			return;
		}
	}

	// TODO: at the moment we don't do full fledged scene reconstruction,
	// only exposing the related models, textures, animation clips, mesh skins
	// as a result, we're ignoring scenes and nodes, and just grabbing
	// the raw resource. To expand later!

	std::vector<Buffer> buffers = ReadBuffers(gltfJson);
	std::vector<BufferView> bufferViews = ReadBufferViews(gltfJson);
	std::vector<Accessor> accessors = ReadAccessors(gltfJson);
	std::vector<Mesh> meshes = ReadMeshes(gltfJson);

	std::vector<Model::IndexType> indices;
	std::vector<Vertex> vertices;

	// concatenating all meshes into a singular one
	// For that we need to ensure they all are of similar layout
	// and while we're at it, collect overall count of vertices+indices
	{
		size_t vertCountTotal = 0;
		size_t indexCountTotal = 0;
		for (const Mesh& mesh : meshes)
		{
			ASSERT_STR(mesh.myAttributes.size() == meshes[0].myAttributes.size()
				&& mesh.myHasIndices == meshes[0].myHasIndices,
				"Meshes format has differing layout, this will cause issues when "
				"concatenating meshes!");
			vertCountTotal += accessors[mesh.myAttributes[0].myAccessor].myCount;
			if (mesh.myHasIndices)
			{
				indexCountTotal += accessors[mesh.myIndexAccessor].myCount;
			}
		}
		vertices.resize(vertCountTotal);
		indices.resize(indexCountTotal);
	}

	// TODO: current implemnentation is too conservative that it does per-element copy
	// even though we're supposed to end up with the same result IF the vertex
	// and Index types are binary the same (so we could just perform a direct memcpy
	// of the whole buffer)
	Model::IndexType indexOffset = 0;
	size_t vertOffset = 0;
	for (const Mesh& mesh : meshes)
	{
		for (const Attribute& attribute : mesh.myAttributes)
		{
			bool hasAttribIndex = false;
			uint32_t attributeSet = 0;
			std::string_view attribName(attribute.myType.c_str(), attribute.myType.size());
			// multiple attributes might be grouped in a set
			// format: <name>_<setNum>
			size_t separatorInd = attribute.myType.find('_');
			if (separatorInd != std::string::npos)
			{
				attribName = std::string_view(attribute.myType.c_str(), separatorInd);
				const char* start = attribute.myType.c_str() + separatorInd + 1;
				const char* end = attribute.myType.c_str() + attribute.myType.size();
				auto [p, errorCode] = std::from_chars(start, end, attributeSet);
				ASSERT(errorCode == std::errc());
				hasAttribIndex = true;
			}

			// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#meshes
			if (attribName == "POSITION")
			{
				const Accessor& posAccessor = accessors[attribute.myAccessor];
				for (size_t i = 0; i < posAccessor.myCount; i++)
				{
					Vertex& vert = vertices[vertOffset + i];
					posAccessor.ReadElem(vert.myPos, i, bufferViews, buffers);
				}
			}
			else if (attribName == "NORMAL")
			{
				const Accessor& normAccessor = accessors[attribute.myAccessor];
				for (size_t i = 0; i < normAccessor.myCount; i++)
				{
					Vertex& vert = vertices[vertOffset + i];
					normAccessor.ReadElem(vert.myNormal, i, bufferViews, buffers);
				}
			}
			else if (attribName == "TEXCOORD")
			{
				ASSERT_STR(hasAttribIndex, "glTF 2.0 standard requires indices!");
				if (attributeSet != 0)
				{
					// skipping other UVs since our vertex only supports 1 UV set
					continue;
				}
				const Accessor& texCoordAccessor = accessors[attribute.myAccessor];
				ASSERT_STR(texCoordAccessor.myComponentType == Accessor::ComponentType::Float,
					"Vertex doesn't support copying in u8 or u16 components!");

				for (size_t i = 0; i < texCoordAccessor.myCount; i++)
				{
					Vertex& vert = vertices[vertOffset + i];
					texCoordAccessor.ReadElem(vert.myUv, i, bufferViews, buffers);
				}
			}
			else
			{
				ASSERT_STR(false, "'%s' semantic attribute NYI!", attribName.data());
			}
		}

		if (mesh.myHasIndices)
		{
			const Accessor& indexAccessor = accessors[mesh.myIndexAccessor];
			for (size_t i = 0; i < indexAccessor.myCount; i++)
			{
				Model::IndexType& index = indices[indexOffset + i];
				switch (indexAccessor.myComponentType)
				{
				case Accessor::ComponentType::UnsignedByte:
				{
					uint8_t readIndex;
					indexAccessor.ReadElem(readIndex, i, bufferViews, buffers);
					index = readIndex;
					break;
				}
				case Accessor::ComponentType::UnsignedShort:
				{
					uint16_t readIndex;
					indexAccessor.ReadElem(readIndex, i, bufferViews, buffers);
					index = readIndex;
					break;
				}
				case Accessor::ComponentType::UnsignedInt:
					indexAccessor.ReadElem(index, i, bufferViews, buffers);
					break;
				}
				ASSERT(static_cast<size_t>(index) + vertOffset < std::numeric_limits<Model::IndexType>::max());
				index += vertOffset; // offset the index since we're concatenating meshes
			}
			indexOffset += indexAccessor.myCount;
		}
		vertOffset += accessors[mesh.myAttributes[0].myAccessor].myCount;
	}


	Model::UploadDescriptor<Vertex> uploadDesc;
	uploadDesc.myVertices = vertices.data();
	uploadDesc.myVertCount = vertices.size();
	uploadDesc.myIndices = indices.data();
	uploadDesc.myIndCount = indices.size();
	uploadDesc.myNextDesc = nullptr;
	uploadDesc.myVertsOwned = false;
	uploadDesc.myIndOwned = false;
	myModel->Update(uploadDesc);
}