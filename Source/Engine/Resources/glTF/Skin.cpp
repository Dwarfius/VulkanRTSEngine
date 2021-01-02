#include "Precomp.h"
#include "Skin.h"

namespace glTF
{
	std::vector<Skin> Skin::Parse(const nlohmann::json& aRoot)
	{
		std::vector<Skin> skins;
		const auto& skinsJsonIter = aRoot.find("skins");
		if (skinsJsonIter == aRoot.end())
		{
			return skins;
		}

		skins.reserve(skinsJsonIter->size());
		for (const nlohmann::json& skinJson : *skinsJsonIter)
		{
			Skin skin;
			skin.myInverseBindMatrices = ReadOptional(skinJson, "inverseBindMatrices", kInvalidInd);
			skin.mySkeleton = ReadOptional(skinJson, "skeleton", kInvalidInd);

			const nlohmann::json& jointsJson = skinJson["joints"];
			skin.myJoints.reserve(jointsJson.size());
			for (const nlohmann::json& jointJson : jointsJson)
			{
				skin.myJoints.push_back(jointJson.get<uint32_t>());
			}

			skin.myName = ReadOptional(skinJson, "name", std::string{});
			skins.push_back(std::move(skin));
		}
		return skins;
	}

	void Skin::ConstructSkeletons(const SkeletonInput& aInputs, 
		std::vector<Skeleton>& aSkeletons, 
		std::unordered_map<uint32_t, Skeleton::BoneIndex>& anIndexMap)
	{
		aSkeletons.reserve(aInputs.mySkins.size());
		std::vector<uint32_t> sortedJoints;
		std::unordered_map<uint32_t, uint32_t> childParentMap;
		for (const Skin& skin : aInputs.mySkins)
		{
			ASSERT_STR(skin.mySkeleton == kInvalidInd, "Custom root bone not yet handled!");

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
			bool foundRoot = false;
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
					ASSERT_STR(!foundRoot, "Uh oh, extra root - how?");
					foundRoot = true;
				}
				
				anIndexMap.insert({ joint, skeleton.GetBoneCount() });
				skeleton.AddBone(parentIndex, node.myTransform);
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

					glm::vec3 scale;
					glm::quat rot;
					glm::vec3 pos;
					glm::vec3 skew;
					glm::vec4 perspective;

					Transform transf;
					if (glm::decompose(inverseMatrix, scale, rot, pos, skew, perspective))
					{
						rot = glm::conjugate(rot);
						transf.SetPos(pos);
						transf.SetRotation(rot);
						transf.SetScale(scale);
					}
					else
					{
						ASSERT_STR(false, "Weird, failed to decompose, inverse matrices will be broken!");
					}
					
					Skeleton::BoneIndex boneInd = static_cast<Skeleton::BoneIndex>(matrixInd);
					skeleton.OverrideInverseBindTransform(boneInd, transf);
				}
			}

			aSkeletons.push_back(skeleton);
		}
	}
}