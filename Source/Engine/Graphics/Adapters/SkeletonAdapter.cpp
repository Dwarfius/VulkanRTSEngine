#include "Precomp.h"
#include "SkeletonAdapter.h"

#include "Graphics/Adapters/AdapterSourceData.h"
#include "GameObject.h"
#include "Animation/AnimationSystem.h"

void SkeletonAdapter::FillUniformBlock(const UniformAdapter::SourceData& aData, UniformBlock& aUB) const
{
	const UniformAdapterSource& data = static_cast<const UniformAdapterSource&>(aData);
	const PoolPtr<Skeleton>& skeletonPtr = data.myGO.GetSkeleton();
	ASSERT_STR(skeletonPtr.IsValid(), "Using adapter for an object that doesn't have a skeleton!");

	// TODO: replace with direct copy - need to adjust Skeleton internals for it!
	const Skeleton* skeleton = skeletonPtr.Get();
	Skeleton::BoneIndex boneCount = skeleton->GetBoneCount();

	aUB.SetUniform(0, 0, boneCount);
	for (Skeleton::BoneIndex index = 0; index < boneCount; index++)
	{
		const Transform& worldTransf = skeleton->GetBoneWorldTransform(index);
		aUB.SetUniform(1, index, worldTransf.GetPos());
		aUB.SetUniform(2, index, worldTransf.GetScale());
		aUB.SetUniform(3, index, worldTransf.GetRotation());
	}
}