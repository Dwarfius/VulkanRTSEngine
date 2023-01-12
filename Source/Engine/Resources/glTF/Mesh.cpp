#include "Precomp.h"
#include "Mesh.h"

#include <charconv>
#include "../../Animation/SkinnedVerts.h"

namespace glTF
{
	void Mesh::ParseItem(const nlohmann::json& aMeshJson, Mesh& aMesh)
	{
		aMesh.myName = ReadOptional(aMeshJson, "name", std::string{});
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
					static_cast<void>(errorCode); // to silence an "unused" warning
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
		std::vector<Transform>& aTransforms,
		std::vector<std::string>& aNames)
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
			Handle<Model> model;
			if (weightsAttribIter == attribs.end())
			{
				// no skinning, so create a simple model 
				ConstructModel<Vertex>(mesh, aInputs, model);
			}
			else
			{
				// skinning present, so construct a model with skinned vertices
				ConstructModel<SkinnedVertex>(mesh, aInputs, model);
			}

			const auto& nodeIter = std::find_if(aInputs.myNodes.begin(), aInputs.myNodes.end(), 
				[&](const Node& aNode) {
					return aNode.myMesh == aModels.size();
				}
			);
			aTransforms.push_back(nodeIter->myWorldTransform);
			aModels.push_back(std::move(model));
			aNames.push_back(mesh.myName);
		}
	}

	void Mesh::ProcessAttribute(const Attribute& anAttribute, const BufferAccessorInputs& aInputs, std::vector<Vertex>& aVertices)
	{
		const std::vector<Buffer>& buffers = aInputs.myBuffers;
		const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
		const Accessor& accessor = aInputs.myAccessors[anAttribute.myAccessor];

		switch (anAttribute.myType)
		{
		case Attribute::Type::Position:
			for (size_t i = 0; i < accessor.myCount; i++)
			{
				Vertex& vert = aVertices[i];
				accessor.ReadElem(vert.myPos, i, bufferViews, buffers);
			}
			break;
		case Attribute::Type::Normal:
			for (size_t i = 0; i < accessor.myCount; i++)
			{
				Vertex& vert = aVertices[i];
				accessor.ReadElem(vert.myNormal, i, bufferViews, buffers);
			}
			break;
		case Attribute::Type::TexCoord:
			ASSERT_STR(anAttribute.mySet == 0, "Vertex only supports 1 texcoord set!");
			if (anAttribute.mySet != 0)
			{
				return;
			}
			ASSERT_STR(accessor.myComponentType == Accessor::ComponentType::Float,
				"Vertex doesn't support copying in u8 or u16 components!");

			for (size_t i = 0; i < accessor.myCount; i++)
			{
				Vertex& vert = aVertices[i];
				accessor.ReadElem(vert.myUv, i, bufferViews, buffers);
			}
			break;
		case Attribute::Type::Joints:
			ASSERT_STR(false, "Unsupported, to construct skinned model call ConstructSkinnedModel!");
			break;
		case Attribute::Type::Weights:
			ASSERT_STR(false, "Unsupported, to construct skinned model call ConstructSkinnedModel!");
			break;
		case Attribute::Type::Tangent:
			// Not yet implemented
			break;
		default:
			ASSERT_STR(false, "Semantic attribute NYI!");
		}
	}

	void Mesh::ProcessAttribute(const Attribute& anAttribute, const BufferAccessorInputs& aInputs, std::vector<SkinnedVertex>& aVertices)
	{
		const std::vector<Buffer>& buffers = aInputs.myBuffers;
		const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
		const Accessor& accessor = aInputs.myAccessors[anAttribute.myAccessor];

		switch (anAttribute.myType)
		{
		case Attribute::Type::Position:
			for (size_t i = 0; i < accessor.myCount; i++)
			{
				SkinnedVertex& vert = aVertices[i];
				accessor.ReadElem(vert.myPos, i, bufferViews, buffers);
			}
			break;
		case Attribute::Type::Normal:
			for (size_t i = 0; i < accessor.myCount; i++)
			{
				SkinnedVertex& vert = aVertices[i];
				accessor.ReadElem(vert.myNormal, i, bufferViews, buffers);
			}
			break;
		case Attribute::Type::TexCoord:
			ASSERT_STR(anAttribute.mySet == 0, "SkinnedVertex only supports 1 texcoord set!");
			if (anAttribute.mySet != 0)
			{
				return;
			}
			ASSERT_STR(accessor.myComponentType == Accessor::ComponentType::Float,
				"Vertex doesn't support copying in u8 or u16 components!");

			for (size_t i = 0; i < accessor.myCount; i++)
			{
				SkinnedVertex& vert = aVertices[i];
				accessor.ReadElem(vert.myUv, i, bufferViews, buffers);
			}
			break;
		case Attribute::Type::Joints:
			ASSERT_STR(anAttribute.mySet == 0, "SkinnedVertex only supports 1 joint set!");
			if (anAttribute.mySet != 0)
			{
				return;
			}

			for (size_t i = 0; i < accessor.myCount; i++)
			{
				SkinnedVertex& vert = aVertices[i];
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
			ASSERT_STR(anAttribute.mySet == 0, "SkinnedVertex only supports 1 weight set!");
			if (anAttribute.mySet != 0)
			{
				return;
			}

			for (size_t i = 0; i < accessor.myCount; i++)
			{
				SkinnedVertex& vert = aVertices[i];
				// weights can be either floats or normalized byte/short
				// so we have to un-normalize them
				accessor.ReadAndDenormalize(vert.myBoneWeights, i, bufferViews, buffers);
			}
			break;
		default:
			ASSERT_STR(false, "Semantic attribute NYI!");
		}
	}
}