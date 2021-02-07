#include "Precomp.h"
#include "Mesh.h"

#include <Graphics/Resources/Model.h>
#include <charconv>
#include "../../Animation/SkinnedVerts.h"

namespace glTF
{
	void Mesh::ParseItem(const nlohmann::json& aMeshJson, Mesh& aMesh)
	{
		const nlohmann::json& primitivesJson = aMeshJson["primitives"];
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

				// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#meshes
				std::string_view attributeName = attribIter.key();
				bool hasAttribIndex = false;
				attribute.mySet = 0;
				// multiple attributes might be grouped in a set
				// format: <name>_<setNum>
				size_t separatorInd = attributeName.find('_');
				if (separatorInd != std::string::npos)
				{
					std::string_view fullName = attributeName;
					attributeName = std::string_view(attributeName.data(), separatorInd);
					const char* start = fullName.data() + separatorInd + 1;
					const char* end = fullName.data() + fullName.size();
					auto [p, errorCode] = std::from_chars(start, end, attribute.mySet);
					ASSERT(errorCode == std::errc());
					hasAttribIndex = true;
				}

				if (attributeName == "POSITION")
				{
					attribute.myType = Attribute::Type::Position;
				}
				else if (attributeName == "NORMAL")
				{
					attribute.myType = Attribute::Type::Normal;
				}
				else if (attributeName == "TANGENT")
				{
					attribute.myType = Attribute::Type::Tangent;
				}
				else if (attributeName == "COLOR")
				{
					ASSERT_STR(hasAttribIndex, "glTF 2.0 standard requires indices!");
					attribute.myType = Attribute::Type::Color;
				}
				else if (attributeName == "TEXCOORD")
				{
					ASSERT_STR(hasAttribIndex, "glTF 2.0 standard requires indices!");
					attribute.myType = Attribute::Type::TexCoord;
				}
				else if (attributeName == "JOINTS")
				{
					ASSERT_STR(hasAttribIndex, "glTF 2.0 standard requires indices!");
					attribute.myType = Attribute::Type::Joints;
				}
				else if (attributeName == "WEIGHTS")
				{
					ASSERT_STR(hasAttribIndex, "glTF 2.0 standard requires indices!");
					attribute.myType = Attribute::Type::Weights;
				}
				else
				{
					ASSERT_STR(false, "'%s' semantic attribute NYI, skipping!", attributeName.data());
					continue;
				}

				attribute.myAccessor = attribIter->get<uint32_t>();
				aMesh.myAttributes.emplace_back(std::move(attribute));
			}

			const auto& indicesJsonIter = primitiveJson.find("indices");
			if (indicesJsonIter != primitiveJson.end())
			{
				aMesh.myHasIndices = true;
				aMesh.myIndexAccessor = indicesJsonIter->get<uint32_t>();
			}
			else
			{
				aMesh.myHasIndices = false;
			}
		}
	}

	void Mesh::ConstructModels(const ModelInputs& aInputs, 
		std::vector<Handle<Model>>& aModels,
		std::vector<Transform>& aTransforms)
	{
		for (const Mesh& mesh : aInputs.myMeshes)
		{
			const std::vector<Attribute>& attribs = mesh.myAttributes;
			const auto& weightsAttribIter = std::find_if(attribs.begin(), attribs.end(),
				[](const Attribute& aAttrib) {
					return aAttrib.myType == Attribute::Type::Weights;
				}
			);
			// Assuming that indexing is always present
			Handle<Model> model = new Model(PrimitiveType::Triangles, nullptr, true);
			if (weightsAttribIter == attribs.end())
			{
				// no skinning, so create a simple model 
				ConstructStaticModel(mesh, aInputs, model);
			}
			else
			{
				// skinning present, so construct a model with skinned vertices
				ConstructSkinnedModel(mesh, aInputs, model);
			}

			const auto& nodeIter = std::find_if(aInputs.myNodes.begin(), aInputs.myNodes.end(), 
				[&](const Node& aNode) {
					return aNode.myMesh == aModels.size();
				}
			);
			aTransforms.push_back(nodeIter->myWorldTransform);
			aModels.push_back(std::move(model));
		}
	}

	// TODO: cleanup some of the common code between this and ConstruckSkinnedModel
	void Mesh::ConstructStaticModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel)
	{
		const std::vector<Buffer>& buffers = aInputs.myBuffers;
		const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
		const std::vector<Accessor>& accessors = aInputs.myAccessors;

		std::vector<Model::IndexType> indices;
		std::vector<Vertex> vertices;

		// just to speed up the process and avoid growth, resize the arrays
		{
			size_t vertCountTotal = 0;
			size_t indexCountTotal = 0;
				
			int posAccessor = kInvalidInd;
			for (int i=0; i<aMesh.myAttributes.size(); i++)
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
			const Accessor& accessor = accessors[attribute.myAccessor];

			switch (attribute.myType)
			{
			case Attribute::Type::Position:
				for (size_t i = 0; i < accessor.myCount; i++)
				{
					Vertex& vert = vertices[i];
					accessor.ReadElem(vert.myPos, i, bufferViews, buffers);
				}
				break;
			case Attribute::Type::Normal:
				for (size_t i = 0; i < accessor.myCount; i++)
				{
					Vertex& vert = vertices[i];
					accessor.ReadElem(vert.myNormal, i, bufferViews, buffers);
				}
				break;
			case Attribute::Type::TexCoord:
				if (attribute.mySet != 0)
				{
					// skipping other UVs since our vertex only supports 1 UV set
					continue;
				}
				ASSERT_STR(accessor.myComponentType == Accessor::ComponentType::Float,
					"Vertex doesn't support copying in u8 or u16 components!");

				for (size_t i = 0; i < accessor.myCount; i++)
				{
					Vertex& vert = vertices[i];
					accessor.ReadElem(vert.myUv, i, bufferViews, buffers);
				}
				break;
			case Attribute::Type::Joints:
				ASSERT_STR(false, "Unsupported, to construct skinned model call ConstructSkinnedModel!");
				break;
			case Attribute::Type::Weights:
				ASSERT_STR(false, "Unsupported, to construct skinned model call ConstructSkinnedModel!");
				break;
			default:
				ASSERT_STR(false, "Semantic attribute NYI!");
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

		Model::UploadDescriptor<Vertex> uploadDesc;
		uploadDesc.myVertices = vertices.data();
		uploadDesc.myVertCount = vertices.size();
		uploadDesc.myIndices = indices.data();
		uploadDesc.myIndCount = indices.size();
		uploadDesc.myNextDesc = nullptr;
		uploadDesc.myVertsOwned = false;
		uploadDesc.myIndOwned = false;
		aModel->Update(uploadDesc);
	}

	void Mesh::ConstructSkinnedModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel)
	{
		const std::vector<Buffer>& buffers = aInputs.myBuffers;
		const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
		const std::vector<Accessor>& accessors = aInputs.myAccessors;

		std::vector<Model::IndexType> indices;
		std::vector<SkinnedVertex> vertices;

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

		// TODO: current implemnentation is too conservative that it does per-element copy
		// even though we're supposed to end up with the same result IF the vertex
		// and Index types are binary the same (so we could just perform a direct memcpy
		// of the whole buffer)
		for (const Mesh::Attribute& attribute : aMesh.myAttributes)
		{
			const Accessor& accessor = accessors[attribute.myAccessor];

			switch (attribute.myType)
			{
			case Attribute::Type::Position:
				for (size_t i = 0; i < accessor.myCount; i++)
				{
					SkinnedVertex& vert = vertices[i];
					accessor.ReadElem(vert.myPos, i, bufferViews, buffers);
				}
				break;
			case Attribute::Type::Normal:
				for (size_t i = 0; i < accessor.myCount; i++)
				{
					SkinnedVertex& vert = vertices[i];
					accessor.ReadElem(vert.myNormal, i, bufferViews, buffers);
				}
				break;
			case Attribute::Type::TexCoord:
				if (attribute.mySet != 0)
				{
					// skipping other UVs since our vertex only supports 1 UV set
					continue;
				}
				ASSERT_STR(accessor.myComponentType == Accessor::ComponentType::Float,
					"Vertex doesn't support copying in u8 or u16 components!");

				for (size_t i = 0; i < accessor.myCount; i++)
				{
					SkinnedVertex& vert = vertices[i];
					accessor.ReadElem(vert.myUv, i, bufferViews, buffers);
				}
				break;
			case Attribute::Type::Joints:
				if (attribute.mySet != 0)
				{
					// skipping other joints since our vertex only supports 
					// a single 4-joint set
					continue;
				}

				for (size_t i = 0; i < accessor.myCount; i++)
				{
					SkinnedVertex& vert = vertices[i];
					if (accessor.myComponentType == Accessor::ComponentType::UnsignedByte)
					{
						uint8_t indices[4];
						accessor.ReadElem(indices, i, bufferViews, buffers);
						for (uint8_t elemIndex = 0; elemIndex < 4; elemIndex++)
						{
							vert.myBoneIndices[elemIndex] = indices[elemIndex];
						}
					}
					else if (accessor.myComponentType == Accessor::ComponentType::UnsignedShort)
					{
						uint16_t indices[4];
						accessor.ReadElem(indices, i, bufferViews, buffers);
						for (uint8_t elemIndex = 0; elemIndex < 4; elemIndex++)
						{
							vert.myBoneIndices[elemIndex] = indices[elemIndex];
						}
					}
					else
					{
						ASSERT(false);
					}
				}
				break;
			case Attribute::Type::Weights:
				if (attribute.mySet != 0)
				{
					// skipping other weight sets since our vertex only supports 1 weights set
					continue;
				}

				for (size_t i = 0; i < accessor.myCount; i++)
				{
					SkinnedVertex& vert = vertices[i];
					// weights can be either floats or normalized byte/short
					// so we have to un-normalize them
					accessor.ReadAndDenormalize(vert.myBoneWeights, i, bufferViews, buffers);
				}
				break;
			default:
				ASSERT_STR(false, "Semantic attribute NYI!");
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

		Model::UploadDescriptor<SkinnedVertex> uploadDesc;
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