#pragma once

#include "Accessor.h"
#include "Node.h"
#include <Graphics/Resources/Model.h>

struct SkinnedVertex;
struct Vertex;

namespace glTF
{
	struct Mesh
	{
		struct Attribute
		{
			enum class Type
			{
				Position,
				Normal,
				Tangent,
				Color,
				TexCoord,
				Joints,
				Weights
			};
			Type myType;
			uint32_t myAccessor;
			uint8_t mySet;
		};

		std::string myName;
		std::vector<Attribute> myAttributes;
		uint32_t myIndexAccessor : 31;
		uint32_t myHasIndices : 1;
		
		static void ParseItem(const nlohmann::json& aMeshJson, Mesh& aMesh);

		// Input set for constructing a Model
		struct ModelInputs : BufferAccessorInputs
		{
			const std::vector<Node>& myNodes;
			const std::vector<Mesh>& myMeshes;
		};
		static void ConstructModels(const ModelInputs& aInputs, 
			std::vector<Handle<Model>>& aModels, 
			std::vector<Transform>& aTransforms, 
			std::vector<std::string>& aNames);

	private:
		template<class T>
		static void ConstructModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel);

		static void ProcessAttribute(const Attribute& anAttribute, const BufferAccessorInputs& aInputs, std::vector<Vertex>& aVertices);
		static void ProcessAttribute(const Attribute& anAttribute, const BufferAccessorInputs& aInputs, std::vector<SkinnedVertex>& aVertices);
	};

	template<class T>
	void Mesh::ConstructModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel)
	{
		const std::vector<Buffer>& buffers = aInputs.myBuffers;
		const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
		const std::vector<Accessor>& accessors = aInputs.myAccessors;

		std::vector<Model::IndexType> indices;
		std::vector<T> vertices;

		glm::vec3 min{ 0.f, 0.f, 0.f };
		glm::vec3 max{ 0.f, 0.f, 0.f };

		// just to speed up the process and avoid growth, resize the arrays
		{
			size_t vertCountTotal = 0;
			size_t indexCountTotal = 0;

			int posAccessor = kInvalidInd;
			for(const glTF::Mesh::Attribute& attribute : aMesh.myAttributes)
			{
				if (attribute.myType == Attribute::Type::Position)
				{
					posAccessor = static_cast<int>(attribute.myAccessor);
					break;
				}
			}
			ASSERT_STR(posAccessor != kInvalidInd, "Failed to find pos attribute!");

			vertCountTotal += accessors[posAccessor].myCount;
			if (aMesh.myHasIndices)
			{
				indexCountTotal += accessors[aMesh.myIndexAccessor].myCount;
			}
			vertices.resize(vertCountTotal);
			indices.resize(indexCountTotal);

			switch (accessors[posAccessor].myComponentType)
			{
			case Accessor::ComponentType::Byte:
			{
				for (uint8_t i = 0; i < 3; i++)
				{
					int8_t data = static_cast<int8_t>(accessors[posAccessor].myMin[i]);
					min[i] = data;
					data = static_cast<int8_t>(accessors[posAccessor].myMax[i]);
					max[i] = data;
				}
				break;
			}
			case Accessor::ComponentType::Short:
			{
				for (uint8_t i = 0; i < 3; i++)
				{
					int16_t data = static_cast<int16_t>(accessors[posAccessor].myMin[i]);
					min[i] = data;
					data = static_cast<int16_t>(accessors[posAccessor].myMax[i]);
					max[i] = data;
				}
				break;
			}
			case Accessor::ComponentType::UnsignedByte: [[fallthrough]];
			case Accessor::ComponentType::UnsignedShort: [[fallthrough]];
			case Accessor::ComponentType::UnsignedInt:
			{
				for (uint8_t i = 0; i < 3; i++)
				{
					min[i] = static_cast<float>(accessors[posAccessor].myMin[i]);
					max[i] = static_cast<float>(accessors[posAccessor].myMax[i]);
				}
				break;
			}
			case Accessor::ComponentType::Float:
			{
				for (uint8_t i = 0; i < 3; i++)
				{
					min[i] = std::bit_cast<float>(accessors[posAccessor].myMin[i]);
					max[i] = std::bit_cast<float>(accessors[posAccessor].myMax[i]);
				}
				break;
			}
			default: ASSERT(false);
			}
		}

		for (const Mesh::Attribute& attribute : aMesh.myAttributes)
		{
			ProcessAttribute(attribute, aInputs, vertices);
		}

		if (aMesh.myHasIndices)
		{
			const Accessor& indexAccessor = accessors[aMesh.myIndexAccessor];
			for (size_t i = 0; i < indexAccessor.myCount; i++)
			{
				Model::IndexType& index = indices[i];
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
			}
		}

		Model::UploadDescriptor<T> uploadDesc;
		uploadDesc.myVertices = vertices.data();
		uploadDesc.myVertCount = vertices.size();
		uploadDesc.myIndices = indices.data();
		uploadDesc.myIndCount = indices.size();
		uploadDesc.myNextDesc = nullptr;
		uploadDesc.myVertsOwned = false;
		uploadDesc.myIndOwned = false;
		aModel->Update(uploadDesc);
		aModel->SetAABB(min, max);
	}
}