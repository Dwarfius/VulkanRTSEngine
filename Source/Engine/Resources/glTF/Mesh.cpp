#include "Precomp.h"
#include "Mesh.h"

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
				attribute.myType = attribIter.key();
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
					return aAttrib.myType == "WEIGHTS_0";
				}
			);
			// Assuming that indexing is always present
			Handle<Model> model = new Model(PrimitiveType::Triangles, nullptr, true);
			if (weightsAttribIter == attribs.end())
			{
				// no skinning, so create a simple model 
				ConstructModel<Vertex>(mesh, aInputs, model, UpdateVertex);
			}
			else
			{
				// skinning present, so construct a model with skinned vertices
				ConstructModel<SkinnedVertex>(mesh, aInputs, model, UpdateSkinnedVertex);
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

	void Mesh::UpdateVertex(Vertex& aVertex, const Attribute& anAttribute, const BufferAccessorInputs& aInputs)
	{
		const std::vector<Buffer>& buffers = aInputs.myBuffers;
		const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
		const std::vector<Accessor>& accessors = aInputs.myAccessors;

		bool hasAttribIndex = false;
		uint32_t attributeSet = 0;
		std::string_view attribName(anAttribute.myType.c_str(), anAttribute.myType.size());
		// multiple attributes might be grouped in a set
		// format: <name>_<setNum>
		size_t separatorInd = anAttribute.myType.find('_');
		if (separatorInd != std::string::npos)
		{
			attribName = std::string_view(anAttribute.myType.c_str(), separatorInd);
			const char* start = anAttribute.myType.c_str() + separatorInd + 1;
			const char* end = anAttribute.myType.c_str() + anAttribute.myType.size();
			auto [p, errorCode] = std::from_chars(start, end, attributeSet);
			ASSERT(errorCode == std::errc());
			hasAttribIndex = true;
		}

		const Accessor& accessor = accessors[anAttribute.myAccessor];

		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#meshes
		if (attribName == "POSITION")
		{
			for (size_t i = 0; i < accessor.myCount; i++)
			{
				accessor.ReadElem(aVertex.myPos, i, bufferViews, buffers);
			}
		}
		else if (attribName == "NORMAL")
		{
			const Accessor& normAccessor = accessors[anAttribute.myAccessor];
			for (size_t i = 0; i < accessor.myCount; i++)
			{
				accessor.ReadElem(aVertex.myNormal, i, bufferViews, buffers);
			}
		}
		else if (attribName == "TEXCOORD")
		{
			ASSERT_STR(hasAttribIndex, "glTF 2.0 standard requires indices!");
			if (attributeSet != 0)
			{
				// skipping other UVs since our vertex only supports 1 UV set
				return;
			}
			ASSERT_STR(accessor.myComponentType == Accessor::ComponentType::Float,
				"Vertex doesn't support copying in u8 or u16 components!");

			for (size_t i = 0; i < accessor.myCount; i++)
			{
				accessor.ReadElem(aVertex.myUv, i, bufferViews, buffers);
			}
		}
		else if (attribName == "JOINTS")
		{
			ASSERT_STR(false, "Unsupported, to construct skinned model call ConstructSkinnedModel!");
		}
		else if (attribName == "WEIGHTS")
		{
			ASSERT_STR(false, "Unsupported, to construct skinned model call ConstructSkinnedModel!");
		}
		else
		{
			ASSERT_STR(false, "'%s' semantic attribute NYI!", attribName.data());
		}
	}

	void Mesh::UpdateSkinnedVertex(SkinnedVertex& aVertex, const Attribute& anAttribute, const BufferAccessorInputs& aInputs)
	{
		const std::vector<Buffer>& buffers = aInputs.myBuffers;
		const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
		const std::vector<Accessor>& accessors = aInputs.myAccessors;

		bool hasAttribIndex = false;
		uint32_t attributeSet = 0;
		std::string_view attribName(anAttribute.myType.c_str(), anAttribute.myType.size());
		// multiple attributes might be grouped in a set
		// format: <name>_<setNum>
		size_t separatorInd = anAttribute.myType.find('_');
		if (separatorInd != std::string::npos)
		{
			attribName = std::string_view(anAttribute.myType.c_str(), separatorInd);
			const char* start = anAttribute.myType.c_str() + separatorInd + 1;
			const char* end = anAttribute.myType.c_str() + anAttribute.myType.size();
			auto [p, errorCode] = std::from_chars(start, end, attributeSet);
			ASSERT(errorCode == std::errc());
			hasAttribIndex = true;
		}

		const Accessor& accessor = accessors[anAttribute.myAccessor];

		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#meshes
		if (attribName == "POSITION")
		{
			for (size_t i = 0; i < accessor.myCount; i++)
			{
				accessor.ReadElem(aVertex.myPos, i, bufferViews, buffers);
			}
		}
		else if (attribName == "NORMAL")
		{
			for (size_t i = 0; i < accessor.myCount; i++)
			{
				accessor.ReadElem(aVertex.myNormal, i, bufferViews, buffers);
			}
		}
		else if (attribName == "TEXCOORD")
		{
			ASSERT_STR(hasAttribIndex, "glTF 2.0 standard requires indices!");
			if (attributeSet != 0)
			{
				// skipping other UVs since our vertex only supports 1 UV set
				return;
			}
			ASSERT_STR(accessor.myComponentType == Accessor::ComponentType::Float,
				"Vertex doesn't support copying in u8 or u16 components!");

			for (size_t i = 0; i < accessor.myCount; i++)
			{
				accessor.ReadElem(aVertex.myUv, i, bufferViews, buffers);
			}
		}
		else if (attribName == "JOINTS")
		{
			if (attributeSet != 0)
			{
				// skipping other joints since our vertex only supports 
				// a single 4-joint set
				return;
			}

			for (size_t i = 0; i < accessor.myCount; i++)
			{
				if (accessor.myComponentType == Accessor::ComponentType::UnsignedByte)
				{
					uint8_t indices[4];
					accessor.ReadElem(indices, i, bufferViews, buffers);
					for (uint8_t elemIndex = 0; elemIndex < 4; elemIndex++)
					{
						aVertex.myBoneIndices[elemIndex] = indices[elemIndex];
					}
				}
				else if (accessor.myComponentType == Accessor::ComponentType::UnsignedShort)
				{
					uint16_t indices[4];
					accessor.ReadElem(indices, i, bufferViews, buffers);
					for (uint8_t elemIndex = 0; elemIndex < 4; elemIndex++)
					{
						aVertex.myBoneIndices[elemIndex] = indices[elemIndex];
					}
				}
				else
				{
					ASSERT(false);
				}
			}
		}
		else if (attribName == "WEIGHTS")
		{
			if (attributeSet != 0)
			{
				// skipping other weight sets since our vertex only supports 1 weights set
				return;
			}

			for (size_t i = 0; i < accessor.myCount; i++)
			{
				// weights can be either floats or normalized byte/short
				// so we have to un-normalize them
				if (accessor.myComponentType == Accessor::ComponentType::Float)
				{
					float weights[4];
					accessor.ReadElem(weights, i, bufferViews, buffers);
					for (uint8_t elemIndex = 0; elemIndex < 4; elemIndex++)
					{
						aVertex.myBoneWeights[elemIndex] = weights[elemIndex];
					}
				}
				else if(accessor.myComponentType == Accessor::ComponentType::UnsignedByte)
				{
					ASSERT_STR(accessor.myIsNormalized, "Must be normalized according to doc!");
					uint8_t weigthsNorm[4];
					accessor.ReadElem(weigthsNorm, i, bufferViews, buffers);
					for (uint8_t elemIndex = 0; elemIndex < 4; elemIndex++)
					{
						aVertex.myBoneWeights[elemIndex] = weigthsNorm[elemIndex] / 255.f;
					}
				}
				else if (accessor.myComponentType == Accessor::ComponentType::UnsignedShort)
				{
					ASSERT_STR(accessor.myIsNormalized, "Must be normalized according to doc!");
					uint16_t weigthsNorm[4];
					accessor.ReadElem(weigthsNorm, i, bufferViews, buffers);
					for (uint8_t elemIndex = 0; elemIndex < 4; elemIndex++)
					{
						aVertex.myBoneWeights[elemIndex] = weigthsNorm[elemIndex] / 65535.f;
					}
				}
			}
		}
		else
		{
			ASSERT_STR(false, "'%s' semantic attribute NYI!", attribName.data());
		}
	}
}