#include "../Precomp.h"
#include "Skeleton.h"

#include <Core/Debug/DebugDrawer.h>
#include <memory_resource>

Skeleton::Skeleton(BoneIndex aCapacity)
{
	myBones.reserve(aCapacity);
}

Skeleton::Skeleton(const std::vector<BoneInitData>& aBones, const Transform& aRootTransf)
{
	ASSERT_STR(aBones.size() < kInvalidIndex, "Too many bones requested!");
	myBones.resize(aBones.size());
	myRootTransform = aRootTransf;

	ASSERT_STR(aBones[0].myParentInd == kInvalidIndex, 
		"First bone must be root and have no parent!");
	for (size_t i = 0; i < aBones.size(); i++)
	{
		ASSERT_STR(aBones[i].myParentInd < i, "Parent bone must appear earlier in the list");
		myBones[i].myLocalTransf = aBones[i].myLocalTransf;
		myBones[i].myParentInd = aBones[i].myParentInd;
	}

	for (size_t i=1; i<myBones.size(); i++)
	{
		Bone& bone = myBones[i];
		bone.myWorldTransf = myBones[bone.myParentInd].myWorldTransf * bone.myLocalTransf;
		// we assume that skeleton is in bind pose when first created
		bone.myInverseBindTransf = bone.myWorldTransf.GetInverted().GetMatrix();
	}
}

void Skeleton::AddBone(BoneIndex aParentIndex, const Transform& aLocalTransf)
{
	ASSERT_STR(aParentIndex < myBones.size() || aParentIndex == kInvalidIndex, 
		"Invalid parent bone index - parent must've been added before this bone.");
	Bone& bone = myBones.emplace_back(aParentIndex, aLocalTransf);
	if (aParentIndex != kInvalidIndex)
	{
		bone.myWorldTransf = myBones[bone.myParentInd].myWorldTransf * bone.myLocalTransf;
	}
	else
	{
		bone.myWorldTransf = bone.myLocalTransf;
	}
	bone.myInverseBindTransf = bone.myWorldTransf.GetInverted().GetMatrix();
}

const Transform& Skeleton::GetBoneLocalTransform(BoneIndex anIndex) const
{
	return myBones[anIndex].myLocalTransf;
}

Transform Skeleton::GetBoneWorldTransform(BoneIndex anIndex) const
{
	return myRootTransform * myBones[anIndex].myWorldTransf;
}

const glm::mat4& Skeleton::GetBoneIverseBindTransform(BoneIndex anIndex) const
{
	return myBones[anIndex].myInverseBindTransf;
}

void Skeleton::SetBoneLocalTransform(BoneIndex anIndex, const Transform& aTransform, bool aUpdateHierarchy /*= true*/)
{
	Bone& bone = myBones[anIndex];
	bone.myLocalTransf = aTransform;

	if (anIndex != 0)
	{
		bone.myWorldTransf = myBones[bone.myParentInd].myWorldTransf * bone.myLocalTransf;
	}
	else
	{
		bone.myWorldTransf = bone.myLocalTransf;
	}

	if (aUpdateHierarchy)
	{
		UpdateHierarchy(anIndex);
	}
}

void Skeleton::SetBoneWorldTransform(BoneIndex anIndex, const Transform& aTransform, bool aUpdateHierarchy /*= true*/)
{
	Bone& bone = myBones[anIndex];
	bone.myWorldTransf = aTransform;

	if (anIndex != 0)
	{
		bone.myLocalTransf = myBones[bone.myParentInd].myWorldTransf * bone.myWorldTransf.GetInverted();
	}
	else
	{
		bone.myLocalTransf = bone.myWorldTransf;
	}

	if (aUpdateHierarchy)
	{
		UpdateHierarchy(anIndex);
	}
}

void Skeleton::OverrideInverseBindTransform(BoneIndex anIndex, const glm::mat4& anInverseMat)
{
	Bone& bone = myBones[anIndex];
	bone.myInverseBindTransf = anInverseMat;
}

void Skeleton::SetRootTransform(const Transform& aTransf)
{
	myRootTransform = aTransf;
}

void Skeleton::DebugDraw(DebugDrawer& aDrawer, const Transform& aWorldTransform) const
{
	for (const Bone& bone : myBones)
	{
		// HACK
		//Transform globalTransf = aWorldTransform * bone.myWorldTransf;
		Transform globalTransf = aWorldTransform * myRootTransform * bone.myWorldTransf;
		aDrawer.AddTransform(globalTransf);
		if (bone.myParentInd != kInvalidIndex)
		{
			// HACK
			//Transform parentGlobalTransf = aWorldTransform * myBones[bone.myParentInd].myWorldTransf;
			Transform parentGlobalTransf = aWorldTransform * myRootTransform * myBones[bone.myParentInd].myWorldTransf;
			const glm::vec3 parentPos = parentGlobalTransf.GetPos();
			const glm::vec3 bonePos = globalTransf.GetPos();
			aDrawer.AddLine(bonePos, parentPos, glm::vec3(1, 1, 0));
		}
	}
}

void Skeleton::UpdateHierarchy(BoneIndex anIndex)
{
	constexpr size_t kBonesMemorySize = 128;
	BoneIndex bonesMemory[kBonesMemorySize];
	std::pmr::monotonic_buffer_resource stackRes(bonesMemory, sizeof(bonesMemory));
	std::pmr::vector<BoneIndex> dirtyIndices(&stackRes);
	dirtyIndices.push_back(anIndex);

	for (BoneIndex i = anIndex + 1; i < myBones.size(); i++)
	{
		Skeleton::BoneIndex parentInd = myBones[i].myParentInd;
		if(std::binary_search(dirtyIndices.begin(), dirtyIndices.end(), parentInd))
		{ 
			Bone& bone = myBones[i];
			bone.myWorldTransf = myBones[parentInd].myWorldTransf * bone.myLocalTransf;
			ASSERT_STR(dirtyIndices.size() + 1 < kBonesMemorySize,
				"Insufficient array size for dity indices, increase!");
			dirtyIndices.push_back(i);
		}
	}
}