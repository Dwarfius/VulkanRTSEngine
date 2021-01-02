#pragma once

#include "Accessor.h"

template<class T> class Handle;
class Model;

namespace glTF
{
	struct Mesh
	{
		struct Attribute
		{
			std::string myType;
			uint32_t myAccessor;
		};

		std::vector<Attribute> myAttributes;
		uint32_t myIndexAccessor : 31;
		uint32_t myHasIndices : 1;
		
		static std::vector<Mesh> Parse(const nlohmann::json& aRootJson);

		// Input set for constructing a Model
		struct ModelInputs : BufferAccessorInputs
		{
			const std::vector<Mesh>& myMeshes;
		};
		static void ConstructModels(const ModelInputs& aInputs, std::vector<Handle<Model>>& aModels);

	private:
		static void ConstructStaticModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel);
		static void ConstructSkinnedModel(const Mesh& aMesh, const BufferAccessorInputs& aInputs, Handle<Model>& aModel);
	};
}