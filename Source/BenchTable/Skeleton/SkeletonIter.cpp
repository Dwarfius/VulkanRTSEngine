#include "Precomp.h"

#include "Skeleton/SkeletonCommon.h"

// This benchmark emulates lookup patterns to see which one is faster
// no grouping vs grouping by parent
// Results: it seems that grouping by parentInd to reduce parent bone lookups
// didn't seem to help - it averages out to be 5-10% worse
// The results also vary (I assume based on branch predictor cache being replaced)
// but no grouping showed most consistent results

static void GroupedByParentCache(benchmark::State& aState)
{
	std::vector<GroupedBone> bones = GroupedBone::ProcessBones(kBoneInputs, aState.range(0));
	Transform res;
	for (auto _ : aState)
	{
		Transform transf;
		Transform parentTransf;
		Skeleton::BoneIndex currParent = Skeleton::kInvalidIndex;
		for (Skeleton::BoneIndex boneIndex = 1; boneIndex < bones.size(); boneIndex++)
		{
			Skeleton::BoneIndex parentInd = bones[boneIndex].myParentInd;
			if (parentInd != currParent)
			{
				currParent = parentInd;
				parentTransf = bones[currParent].myWorldTransf;
			}
			benchmark::DoNotOptimize(parentTransf);
			transf = parentTransf * bones[boneIndex].myLocalTransf;
			benchmark::DoNotOptimize(transf);
		}
		benchmark::DoNotOptimize(res = transf);
	}
}

static void GroupedByParent(benchmark::State& aState)
{
	std::vector<GroupedBone> bones = GroupedBone::ProcessBones(kBoneInputs, aState.range(0));
	Transform res;
	for (auto _ : aState)
	{
		Transform transf;
		for (Skeleton::BoneIndex boneIndex = 1; boneIndex < bones.size(); boneIndex++)
		{
			Skeleton::BoneIndex parentInd = bones[boneIndex].myParentInd;
			Transform parentTransf = bones[parentInd].myWorldTransf;
			benchmark::DoNotOptimize(parentTransf);
			transf = parentTransf * bones[boneIndex].myLocalTransf;
			benchmark::DoNotOptimize(transf);
		}
		benchmark::DoNotOptimize(res = transf);
	}
}

static void RandomLookback(benchmark::State& aState)
{
	std::vector<Bone> bones = Bone::ProcessBones(kBoneInputs, aState.range(0));
	Transform res;
	for (auto _ : aState)
	{
		Transform transf;
		for (Skeleton::BoneIndex boneIndex = 1; boneIndex < bones.size(); boneIndex++)
		{
			const Transform& parentLocal = bones[bones[boneIndex].myParentInd].myWorldTransf;
			benchmark::DoNotOptimize(parentLocal);
			transf = parentLocal * bones[boneIndex].myLocalTransf;
			benchmark::DoNotOptimize(transf);
		}
		benchmark::DoNotOptimize(res = transf);
	}
}

BENCHMARK(GroupedByParentCache)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);
BENCHMARK(GroupedByParent)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);
BENCHMARK(RandomLookback)->Arg(32)->Arg(64)->Arg(128)->Arg(512)->Arg(1024);