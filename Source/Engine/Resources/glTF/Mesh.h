#pragma once

#include "Accessor.h"
#include "Node.h"

template<class T> class Handle;
class Model;

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
		static void ConstructStaticModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel);
		static void ConstructSkinnedModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel);
	};
}