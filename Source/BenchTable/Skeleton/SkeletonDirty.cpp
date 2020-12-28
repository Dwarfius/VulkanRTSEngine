#include "Precomp.h"

#include "Skeleton/SkeletonCommon.h"
#include <memory_resource>

// This test aims to see how important it is to try 
// to group together bones when trying to dirty the hierarchy
// Results: Grouping and PMR-using iteratation scales better than
// recursive or ungrouped iterations - on 1024 bones with 10% to dirty
// it was 3 times faster, while on smaller sets (128 bones) it had very
// similar performance

struct DirtyEntry
{
	bool myIsDirty;
};

static void GroupedPMR(benchmark::State& aState)
{
	std::vector<GroupedBone> bones = GroupedBone::ProcessBones(kBoneInputs, aState.range(0));
	const int64_t dirtyCount = (int64_t)(aState.range(0) * 0.1);
	std::vector<DirtyEntry> dirtyEntries;
	dirtyEntries.resize(bones.size());

	// TODO: bench adding an early out here, before adding to the queue!
	constexpr size_t kBonesMemorySize = 512;
	Skeleton::BoneIndex bonesMemory[kBonesMemorySize];
	std::pmr::monotonic_buffer_resource stackRes(bonesMemory, sizeof(bonesMemory));
	std::pmr::deque<Skeleton::BoneIndex> bonesToVisit(&stackRes);

	for (auto _ : aState)
	{
		aState.PauseTiming();
		std::memset(dirtyEntries.data(), 0, dirtyEntries.size() * sizeof(DirtyEntry));
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

			DirtyEntry& entry = dirtyEntries[boneIndex];
			if (entry.myIsDirty)
			{
				// the hierarchy bellow has already been dirtied
				// so skip this one
				continue;
			}
			// dirty current bone
			entry.myIsDirty = true;

			const GroupedBone& bone = bones[boneIndex];
			ASSERT_STR(bonesToVisit.size() + bone.myChildCount < kBonesMemorySize,
				"Not enough space allocated on the stack to dirty "
				"the hierarchy! Try increasing the size!");

			// and schedule all of the children for dirtying
			for (Skeleton::BoneIndex childOffset = 0;
				childOffset < bone.myChildCount;
				childOffset++)
			{
				bonesToVisit.push_front(bone.myFirstChildInd + childOffset);
			}
		}
	}
}

static void Dirty(const std::vector<GroupedBone>& aBones, std::vector<DirtyEntry>& aEntries, Skeleton::BoneIndex aIndex)
{
	if (aEntries[aIndex].myIsDirty)
	{
		return;
	}
	aEntries[aIndex].myIsDirty = true;

	const GroupedBone& bone = aBones[aIndex];
	for (Skeleton::BoneIndex childOffset = 0;
		childOffset < bone.myChildCount;
		childOffset++)
	{
		Dirty(aBones, aEntries, bone.myFirstChildInd + childOffset);
	}
}

static void Grouped(benchmark::State& aState)
{
	std::vector<GroupedBone> bones = GroupedBone::ProcessBones(kBoneInputs, aState.range(0));
	const int64_t dirtyCount = (int64_t)(aState.range(0) * 0.1);
	std::vector<DirtyEntry> dirtyEntries;
	dirtyEntries.resize(bones.size());
	for (auto _ : aState)
	{
		aState.PauseTiming();
		std::memset(dirtyEntries.data(), 0, dirtyEntries.size() * sizeof(DirtyEntry));
		aState.ResumeTiming();

		// dirty all bones
		for (int64_t i = 0; i < dirtyCount; i++)
		{
			Skeleton::BoneIndex boneToDirty = kIndices[i];
			Dirty(bones, dirtyEntries, boneToDirty);
		}
	}
}

static void UnGrouped(benchmark::State& aState)
{
	std::vector<Bone> bones = Bone::ProcessBones(kBoneInputs, aState.range(0));
	const int64_t dirtyCount = (int64_t)(aState.range(0) * 0.1);
	std::vector<DirtyEntry> dirtyEntries;
	dirtyEntries.resize(bones.size());
	for (auto _ : aState)
	{
		aState.PauseTiming();
		std::memset(dirtyEntries.data(), 0, dirtyEntries.size() * sizeof(DirtyEntry));
		aState.ResumeTiming();

		// dirty all bones
		for (int64_t i = 0; i < dirtyCount; i++)
		{
			Skeleton::BoneIndex boneToDirty = kIndices[i];
			dirtyEntries[boneToDirty].myIsDirty = true;
		}

		// mark the hierarchy for dirty
		for (int64_t i = 0; i < bones.size(); i++)
		{
			const Bone& bone = bones[i];
			if (i != 0 && dirtyEntries[bone.myParentInd].myIsDirty)
			{
				dirtyEntries[i].myIsDirty = true;
			}
		}
	}
}

BENCHMARK(GroupedPMR)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);;
BENCHMARK(Grouped)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);;
BENCHMARK(UnGrouped)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);;