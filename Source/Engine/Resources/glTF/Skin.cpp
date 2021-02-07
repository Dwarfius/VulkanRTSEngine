#include "Precomp.h"
#include "Skin.h"

namespace glTF
{
	void Skin::ParseItem(const nlohmann::json& aSkinJson, Skin& aSkin)
	{
		aSkin.myInverseBindMatrices = ReadOptional(aSkinJson, "inverseBindMatrices", kInvalidInd);
		aSkin.mySkeleton = ReadOptional(aSkinJson, "skeleton", kInvalidInd);

		const nlohmann::json& jointsJson = aSkinJson["joints"];
		aSkin.myJoints = jointsJson.get<std::vector<uint32_t>>();

		aSkin.myName = ReadOptional(aSkinJson, "name", std::string{});
	}

	void Skin::ConstructSkeletons(const SkeletonInput& aInputs, 
		std::vector<Skeleton>& aSkeletons, 
		std::unordered_map<uint32_t, Skeleton::BoneIndex>& anIndexMap,
		std::vector<Transform>& aTransforms)
	{
		aSkeletons.reserve(aInputs.mySkins.size());
		std::vector<uint32_t> sortedJoints;
		std::unordered_map<uint32_t, uint32_t> childParentMap;
		for (const Skin& skin : aInputs.mySkins)
		{
			Skeleton skeleton(static_cast<Skeleton::BoneIndex>(skin.myJoints.size()));
			
			// first have to pre-process the joints to determine their hierarchy
			// This can't be done during the node parsing stage as there's no info
			// on whether node belongs to skin hierarchy or not
			sortedJoints.reserve(skin.myJoints.size());
			sortedJoints.insert(sortedJoints.end(), skin.myJoints.begin(), skin.myJoints.end());
			std::sort(sortedJoints.begin(), sortedJoints.end());

			for (uint32_t joint : skin.myJoints)
			{
				const Node& node = aInputs.myNodes[joint];
				for (uint32_t child : node.myChildren)
				{
					// TODO: likely
					if (std::binary_search(sortedJoints.begin(), sortedJoints.end(), child))
					{
						childParentMap.emplace(child, joint);
					}
				}
			}
			sortedJoints.clear();

			// we're now ready to reconstruct the hierarchy as a skeleton
			Skeleton::BoneIndex foundRoot = Skeleton::kInvalidIndex;
			for (uint32_t joint : skin.myJoints)
			{
				const Node& node = aInputs.myNodes[joint];
				const auto& jointParentIter = childParentMap.find(joint);
				Skeleton::BoneIndex parentIndex = Skeleton::kInvalidIndex;
				if (jointParentIter != childParentMap.end())
				{
					parentIndex = anIndexMap.at(jointParentIter->second);
				}
				else
				{
					ASSERT_STR(foundRoot == Skeleton::kInvalidIndex, 
						"Uh oh, extra root - how?");
					foundRoot = static_cast<Skeleton::BoneIndex>(joint);
				}
				
				anIndexMap.insert({ joint, skeleton.GetBoneCount() });

				skeleton.AddBone(parentIndex, node.myTransform);
			}

			auto nodeIter = std::find_if(aInputs.myNodes.begin(), aInputs.myNodes.end(), [&](const Node& aNode)
			{
				return aNode.mySkin == aSkeletons.size();
			});

			if (skin.mySkeleton != kInvalidInd
				&& skin.mySkeleton != foundRoot)
			{
				// baking in the extra root transform into the gameobject transform
				Transform& transf = aTransforms[aSkeletons.size()];
				transf = transf * aInputs.myNodes[skin.mySkeleton].myWorldTransform;
			}
			
			// clearing the map, as the joints can be reused across skins
			childParentMap.clear();

			if (skin.myInverseBindMatrices != kInvalidInd)
			{
				const Accessor& matricesAccessor = aInputs.myAccessors[skin.myInverseBindMatrices];
				ASSERT_STR(matricesAccessor.myCount == skin.myJoints.size(), 
					"Per specification, if inverse bind matrices are provided, "
					"there must be one for each joint!");
				for (size_t matrixInd = 0; matrixInd < matricesAccessor.myCount; matrixInd++)
				{
					glm::mat4 inverseMatrix;
					matricesAccessor.ReadElem(inverseMatrix, matrixInd, 
						aInputs.myBufferViews, aInputs.myBuffers);
					
					Skeleton::BoneIndex boneInd = static_cast<Skeleton::BoneIndex>(matrixInd);
					skeleton.OverrideInverseBindTransform(boneInd, inverseMatrix);
				}
			}

			aSkeletons.push_back(skeleton);
		}
	}
}