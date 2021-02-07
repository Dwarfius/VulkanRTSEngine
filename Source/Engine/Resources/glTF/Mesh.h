#pragma once

#include "Accessor.h"
#include "Node.h"
#include <Graphics/Resources/Model.h>

struct SkinnedVertex;

namespace glTF
{
	struct Mesh
	{
		struct Attribute
		{
			// TODO: Replace with an enum
			std::string myType;
			uint32_t myAccessor;
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
		static void UpdateVertex(Vertex& aVertex, const Attribute& anAttribute, const BufferAccessorInputs& aInputs);
		static void UpdateSkinnedVertex(SkinnedVertex& aVertex, const Attribute& anAttribute, const BufferAccessorInputs& aInputs);

		template<class TVert, class TUpdateVert>
		static void ConstructModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel, TUpdateVert anUpdateLambda)
		{
			const std::vector<Buffer>& buffers = aInputs.myBuffers;
			const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
			const std::vector<Accessor>& accessors = aInputs.myAccessors;

			std::vector<Model::IndexType> indices;
			std::vector<TVert> vertices;

			// just to speed up the process and avoid growth, resize the arrays
			{
				size_t vertCountTotal = 0;
				size_t indexCountTotal = 0;

				int posAccessor = kInvalidInd;
				for (int i = 0; i < aMesh.myAttributes.size(); i++)
				{
					if (aMesh.myAttributes[i].myType == "POSITION")
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

			for (TVert& vertex : vertices)
			{
				for (const Mesh::Attribute& attribute : aMesh.myAttributes)
				{
					anUpdateLambda(vertex, attribute, aInputs);
				}
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

			Model::UploadDescriptor<TVert> uploadDesc;
			uploadDesc.myVertices = vertices.data();
			uploadDesc.myVertCount = vertices.size();
			uploadDesc.myIndices = indices.data();
			uploadDesc.myIndCount = indices.size();
			uploadDesc.myNextDesc = nullptr;
			uploadDesc.myVertsOwned = false;
			uploadDesc.myIndOwned = false;
			aModel->Update(uploadDesc);
		}
	};
}