#include "Precomp.h"
#include "SkeletonAdapter.h"

#include "Graphics/Adapters/AdapterSourceData.h"
#include "GameObject.h"
#include "Animation/AnimationSystem.h"

void SkeletonAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const UniformAdapterSource& data = static_cast<const UniformAdapterSource&>(aData);
	ASSERT_STR(data.myGO, "GameObject needed to access skeleton");
	const PoolPtr<Skeleton>& skeletonPtr = data.myGO->GetSkeleton();
	ASSERT_STR(skeletonPtr.IsValid(), "Using adapter for an object that doesn't have a skeleton!");

	const Skeleton* skeleton = skeletonPtr.Get();
	Skeleton::BoneIndex boneCount = skeleton->GetBoneCount();
	for (Skeleton::BoneIndex index = 0; index < boneCount; index++)
	{
		const glm::mat4& worldBoneTransf = skeleton->GetBoneWorldTransform(index).GetMatrix();
		const glm::mat4& inverseBindPoseTransf = skeleton->GetBoneIverseBindTransform(index);
		glm::mat4 skinningSkeletonSpace = worldBoneTransf * inverseBindPoseTransf;

		aUB.SetUniform(0, index, skinningSkeletonSpace);
	}
}