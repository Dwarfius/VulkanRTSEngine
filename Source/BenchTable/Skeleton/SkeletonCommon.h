#pragma once

#include <random>
#include <Core/Transform.h>
#include <Engine/Animation/Skeleton.h>

struct BoneInput
{
	Transform myLocalTransf{};
	Skeleton::BoneIndex myParentInd = Skeleton::kInvalidIndex;
};
constexpr static size_t kMaxSize = 1024;
const static std::vector<BoneInput> kBoneInputs = [] {
	std::random_device realRandSeed;
	std::mt19937 engine(realRandSeed());
	std::uniform_int_distribution<Skeleton::BoneIndex> distrib;

	std::vector<BoneInput> inputs;
	inputs.resize(kMaxSize);
	for (size_t i = 1; i < kMaxSize; i++)
	{
		std::uniform_int_distribution<Skeleton::BoneIndex>::param_type newParams(0, i - 1);
		distrib.param(newParams);
		inputs[i].myParentInd = distrib(engine);
	}

	return inputs;
} ();
const static std::vector<Skeleton::BoneIndex> kIndices = [] {
	std::random_device realRandSeed;
	std::mt19937 engine(realRandSeed());
	std::uniform_int_distribution<Skeleton::BoneIndex> distrib;

	std::vector<Skeleton::BoneIndex> indices;
	indices.resize(kMaxSize);
	indices[0] = 0;
	for (Skeleton::BoneIndex i = 1; i < kMaxSize; i++)
	{
		std::uniform_int_distribution<Skeleton::BoneIndex>::param_type newParams(1, i);
		distrib.param(newParams);
		indices[i] = distrib(engine);
	}
	return indices;
}();

struct GroupedBone
{
	Transform myLocalTransf{};
	Transform myWorldTransf{};
	// unused, padding
	Transform myInverseBindTransf{};
	// ====
	Skeleton::BoneIndex myParentInd = Skeleton::kInvalidIndex;
	// unused, padding
	Skeleton::BoneIndex myFirstChildInd = Skeleton::kInvalidIndex;
	Skeleton::BoneIndex myChildCount = 0;
	// ====

	static std::vector<GroupedBone> ProcessBones(const std::vector<BoneInput>& aInputs, size_t aCount)
	{
		std::vector<GroupedBone> bones;
		bones.resize(aCount);

		// copying all the info
		for (size_t i = 0; i < aCount; i++)
		{
			bones[i].myLocalTransf = aInputs[i].myLocalTransf;
			bones[i].myParentInd = aInputs[i].myParentInd;
		}

		// grouping by parent
		auto sortStart = bones.begin();
		std::advance(bones.begin(), 1);
		std::sort(sortStart, bones.end(), [](const GroupedBone& left, const GroupedBone& right) {
			if (left.myParentInd == Skeleton::kInvalidIndex)
			{
				return true;
			}
			else if (right.myParentInd == Skeleton::kInvalidIndex)
			{
				return false;
			}
			return left.myParentInd < right.myParentInd;
		});

		// find grouping indices
		Skeleton::BoneIndex currStart = 0;
		Skeleton::BoneIndex currCount = 0;
		Skeleton::BoneIndex currParent = Skeleton::kInvalidIndex;
		for (Skeleton::BoneIndex i = 0; i < bones.size(); i++)
		{
			GroupedBone& bone = bones[i];
			if (bone.myParentInd != currParent)
			{
				for (Skeleton::BoneIndex j = currStart; j < currStart + currCount; j++)
				{
					bones[j].myFirstChildInd = currStart;
					bones[j].myChildCount = currCount;
				}

				currStart = i;
				currCount = 0;
				currParent = bone.myParentInd;
			}
			else
			{
				currCount++;
			}
		}
		for (Skeleton::BoneIndex j = currStart; j < currStart + currCount; j++)
		{
			bones[j].myFirstChildInd = currStart;
			bones[j].myChildCount = currCount + 1;
		}
		return bones;
	}
};

struct Bone
{
	Transform myLocalTransf{};
	Transform myWorldTransf{};
	// unused, padding
	Transform myInverseBindTransf{};
	// ====
	Skeleton::BoneIndex myParentInd = Skeleton::kInvalidIndex;

	static std::vector<Bone> ProcessBones(const std::vector<BoneInput>& aInputs, size_t aCount)
	{
		std::vector<Bone> bones;
		bones.resize(aCount);
		for (size_t i = 0; i < aCount; i++)
		{
			bones[i].myLocalTransf = aInputs[i].myLocalTransf;
			bones[i].myParentInd = aInputs[i].myParentInd;
		}
		return bones;
	}
};