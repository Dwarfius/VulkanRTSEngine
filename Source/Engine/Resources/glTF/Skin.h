#pragma once

#include "Accessor.h"
#include "Node.h"

#include "Animation/Skeleton.h"

namespace glTF
{
	struct Skin
	{
		std::vector<uint32_t> myJoints; // req
		std::string myName;
		int myInverseBindMatrices; // index of accessor of mat4s
		int mySkeleton; // index of node used as skeleton root

		static void ParseItem(const nlohmann::json& aSkinJson, Skin& aSkin);

		struct SkeletonInput : BufferAccessorInputs
		{
			const std::vector<Node>& myNodes;
			const std::vector<Skin>& mySkins;
		};
		static void ConstructSkeletons(const SkeletonInput& aInputs, 
			std::vector<Skeleton>& aSkeletons,
			std::unordered_map<uint32_t, Skeleton::BoneIndex>& anIndexMap,
			std::vector<Transform>& aTransforms,
			std::vector<std::string>& aNames);
	};
}