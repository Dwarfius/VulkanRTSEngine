#include "Precomp.h"

#include "Skeleton/SkeletonCommon.h"
#include <memory_resource>

// This test aims to see how important it is to try 
// to group together bones when trying to dirty the hierarchy
// Results: Grouping and PMR-using iteratation scales better than
// recursive or ungrouped iterations - on 1024 bones with 10% to dirty
// PMR was 3 times faster than Dirty_Ungrouped, and 2 time faster than Dirty_Grouped
// while on smaller sets (128 bones) it had very similar performance

template<class T>
struct DirtyTemplate
{
	T myBone;
	bool myIsDirty;
};

static void Dirty_GroupedPMR(benchmark::State& aState)
{
	using DirtyBone = DirtyTemplate<GroupedBone>;
	const int64_t dirtyCount = (int64_t)(aState.range(0) * 0.1);
	std::vector<DirtyBone> bones;

	{
		std::vector<GroupedBone> origBones = GroupedBone::ProcessBones(kBoneInputs, aState.range(0));
		bones.resize(origBones.size());
		for (size_t i = 0; i < origBones.size(); i++)
		{
			bones[i].myBone = origBones[i];
			bones[i].myIsDirty = false;
		}
	}

	constexpr size_t kBonesMemorySize = 512;
	Skeleton::BoneIndex bonesMemory[kBonesMemorySize];
	std::pmr::monotonic_buffer_resource stackRes(bonesMemory, sizeof(bonesMemory));
	std::pmr::deque<Skeleton::BoneIndex> bonesToVisit(&stackRes);

	for (auto _ : aState)
	{
		aState.PauseTiming();
		for (DirtyBone& bone : bones)
		{
			bone.myIsDirty = false;
		}
		aState.ResumeTiming();

		// dirty all bones
		for (int64_t i = 0; i < dirtyCount; i++)
		{
			Skeleton::BoneIndex boneToDirty = kIndices[i];
			bonesToVisit.push_front(boneToDirty);
		}

		while (bonesToVisit.size())
		{
			const Skeleton::BoneIndex boneIndex = bonesToVisit.front();
			bonesToVisit.pop_back();

			DirtyBone& bone = bones[boneIndex];
			if (bone.myIsDirty)
			{
				// the hierarchy bellow has already been dirtied
				// so skip this one
				continue;
			}
			// dirty current bone
			bone.myIsDirty = true;

			ASSERT_STR(bonesToVisit.size() + bone.myBone.myChildCount < kBonesMemorySize,
				"Not enough space allocated on the stack to dirty "
				"the hierarchy! Try increasing the size!");

			// and schedule all of the children for dirtying
			for (Skeleton::BoneIndex childOffset = 0;
				childOffset < bone.myBone.myChildCount;
				childOffset++)
			{
				bonesToVisit.push_front(bone.myBone.myFirstChildInd + childOffset);
			}
		}
	}
}

static void Dirty(std::vector<DirtyTemplate<GroupedBone>>& aBones, Skeleton::BoneIndex aIndex)
{
	using DirtyBone = DirtyTemplate<GroupedBone>;

	if (aBones[aIndex].myIsDirty)
	{
		return;
	}
	aBones[aIndex].myIsDirty = true;

	DirtyBone& bone = aBones[aIndex];
	for (Skeleton::BoneIndex childOffset = 0;
		childOffset < bone.myBone.myChildCount;
		childOffset++)
	{
		Dirty(aBones, bone.myBone.myFirstChildInd + childOffset);
	}
}

static void Dirty_Grouped(benchmark::State& aState)
{
	using DirtyBone = DirtyTemplate<GroupedBone>;
	const int64_t dirtyCount = (int64_t)(aState.range(0) * 0.1);
	std::vector<DirtyBone> bones;

	{
		std::vector<GroupedBone> origBones = GroupedBone::ProcessBones(kBoneInputs, aState.range(0));
		bones.resize(origBones.size());
		for (size_t i = 0; i < origBones.size(); i++)
		{
			bones[i].myBone = origBones[i];
			bones[i].myIsDirty = false;
		}
	}

	for (auto _ : aState)
	{
		aState.PauseTiming();
		for (DirtyBone& bone : bones)
		{
			bone.myIsDirty = false;
		}
		aState.ResumeTiming();

		// dirty all bones
		for (int64_t i = 0; i < dirtyCount; i++)
		{
			Skeleton::BoneIndex boneToDirty = kIndices[i];
			Dirty(bones, boneToDirty);
		}
	}
}

static void Dirty_UnGrouped(benchmark::State& aState)
{
	using DirtyBone = DirtyTemplate<Bone>;
	const int64_t dirtyCount = (int64_t)(aState.range(0) * 0.1);
	std::vector<DirtyBone> bones;

	{
		std::vector<Bone> origBones = Bone::ProcessBones(kBoneInputs, aState.range(0));
		bones.resize(origBones.size());
		for (size_t i = 0; i < origBones.size(); i++)
		{
			bones[i].myBone = origBones[i];
			bones[i].myIsDirty = false;
		}
	}

	for (auto _ : aState)
	{
		aState.PauseTiming();
		for(DirtyBone& bone : bones)
		{
			bone.myIsDirty = false;
		}
		aState.ResumeTiming();

		// dirty all bones
		for (int64_t i = 0; i < dirtyCount; i++)
		{
			Skeleton::BoneIndex boneToDirty = kIndices[i];
			bones[boneToDirty].myIsDirty = true;
		}

		// mark the hierarchy for dirty
		for (int64_t i = 0; i < bones.size(); i++)
		{
			const DirtyBone& bone = bones[i];
			if (i != 0 && bones[bone.myBone.myParentInd].myIsDirty)
			{
				bones[i].myIsDirty = true;
			}
		}
	}
}

BENCHMARK(Dirty_GroupedPMR)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);;
BENCHMARK(Dirty_Grouped)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);;
BENCHMARK(Dirty_UnGrouped)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);;