#pragma once

#include "Accessor.h"
#include "Node.h"

template<class T> class Handle;
class Model;
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
		static void ConstructModels(const ModelInputs& aInputs, std::vector<Handle<Model>>& aModels, std::vector<Transform>& aTransforms);

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

		// just to speed up the process and avoid growth, resize the arrays
		{
			size_t vertCountTotal = 0;
			size_t indexCountTotal = 0;

			int posAccessor = kInvalidInd;
			for (int i = 0; i < aMesh.myAttributes.size(); i++)
			{
				if (aMesh.myAttributes[i].myType == Attribute::Type::Position)
				{
					posAccessor = i;
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
	}
}