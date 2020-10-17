#include "../Precomp.h"
#include "Skeleton.h"

#include "Animation/AnimationController.h"

#include <Core/Debug/DebugDrawer.h>
#include <memory_resource>

Skeleton::Skeleton(BoneIndex aCapacity)
{
	ASSERT_STR(aCapacity < kInvalidIndex, "Too many bones requested!");
	myBones.reserve(aCapacity);
}

Skeleton::~Skeleton()
{
	if (myController)
	{
		delete myController;
	}
}

void Skeleton::AddBone(BoneIndex aParentIndex, const Transform& aLocalTransf)
{
	if (aParentIndex == kInvalidIndex)
	{
		ASSERT_STR(myBones.size() == 0, "Root bone must be added first!");
		Bone& rootBone = myBones.emplace_back(aParentIndex, aLocalTransf);
		rootBone.myFirstChildInd = 1;
	}
	else
	{
		ASSERT_STR(aParentIndex < myBones.size(), "Root bone must be added first!");
		// we're about to add a child bone, so have to find an appropriate location for it
		// maintaining the breadth-first order
		Bone& parentBone = myBones[aParentIndex];
		parentBone.myChildCount++;
		BoneIndex childGroupStart = parentBone.myFirstChildInd;

		// We will always insert at the start - this makes all the book-keeping
		// easier - we only need to shift the start indices of parent's children
		auto childBoneIter = myBones.emplace(myBones.begin() + childGroupStart, aParentIndex, aLocalTransf);
		Bone& childBone = *childBoneIter;
		// since we insert at the start of the parent group, the free child index will always
		// be at the end of the current group (parent's children)
		childBone.myFirstChildInd = parentBone.myFirstChildInd + parentBone.myChildCount;

		// since we inserted in the "middle", update the indexes
		// of all the bones after us with a simple shift to the right
		for (BoneIndex boneIndex = childGroupStart + 1; 
				boneIndex < myBones.size(); 
				boneIndex++)
		{
			// we inserted one - so just shift by one
			myBones[boneIndex].myFirstChildInd++; 
		}
	}
}

const Transform& Skeleton::GetBoneLocalTransform(BoneIndex anIndex) const
{
	return myBones[anIndex].myLocalTransf;
}

const Transform& Skeleton::GetBoneWorldTransform(BoneIndex anIndex) const
{
	return myBones[anIndex].myWorldTransf;
}

void Skeleton::SetBoneLocalTransform(BoneIndex anIndex, const Transform& aTransform)
{
	Bone& bone = myBones[anIndex];
	bone.myLocalTransf = aTransform;
	DirtyHierarchy(anIndex, false);
}

void Skeleton::SetBoneWorldTransform(BoneIndex anIndex, const Transform& aTransform)
{
	Bone& bone = myBones[anIndex];
	bone.myWorldTransf = aTransform;
	DirtyHierarchy(anIndex, true);
}

void Skeleton::AddController()
{
	myController = new AnimationController();
}

void Skeleton::Update(float aDeltaTime)
{
	if (myController && myController->NeedsUpdate())
	{
		myController->Update(*this, aDeltaTime);
	}

	if (!myNeedsUpdate)
	{
		return;
	}

	// Current method relies on the fact that bones in breadth-first order
	UpdateBone(0, {});
	for (BoneIndex boneIndex = 1; boneIndex < myBones.size(); boneIndex++)
	{
		ASSERT(!myBones[myBones[boneIndex].myParentInd].myIsWorldDirty);
		// using look back iteration... this relies on the fact that 
		// the values are probably saved in the cache already
		// TODO: bench a version that only does memory access if parentInd changed
		// should have better cache utilization, at a price of misprediction
		const Transform& parentLocal = myBones[myBones[boneIndex].myParentInd].myWorldTransf;
		UpdateBone(boneIndex, parentLocal);
	}
	myNeedsUpdate = false;
}

void Skeleton::DebugDraw(DebugDrawer& aDrawer, const Transform& aWorldTransform) const
{
	for (const Bone& bone : myBones)
	{
		Transform globalTransf = aWorldTransform * bone.myWorldTransf;
		aDrawer.AddTransform(globalTransf);
		if (bone.myParentInd != kInvalidIndex)
		{
			Transform parentGlobalTransf = aWorldTransform * myBones[bone.myParentInd].myWorldTransf;
			const glm::vec3 parentPos = parentGlobalTransf.GetPos();
			const glm::vec3 bonePos = globalTransf.GetPos();
			aDrawer.AddLine(bonePos, parentPos, glm::vec3(1, 1, 0));
		}
	}
}

void Skeleton::DirtyHierarchy(BoneIndex indexOfRoot, bool aIsWorldDirty)
{
	// TODO: bench adding an early out here, before adding to the queue!
	constexpr size_t kBonesMemorySize = 48;
	BoneIndex bonesMemory[kBonesMemorySize];
	std::pmr::monotonic_buffer_resource stackRes(bonesMemory, sizeof(bonesMemory));
	std::pmr::deque<BoneIndex> bonesToVisit(&stackRes);

	bonesToVisit.push_front(indexOfRoot);
	while (bonesToVisit.size()) 
	{
		const BoneIndex boneIndex = bonesToVisit.back();
		bonesToVisit.pop_back();

		Bone& bone = myBones[boneIndex];
		if (bone.myIsLocalDirty)
		{
			// the hierarchy bellow has already been dirtied
			// so skip this one
			continue;
		}
		// dirty current bone
		bone.myIsLocalDirty = true;

		// and schedule all of the children for dirtying
		for (BoneIndex childOffset = 0;
			childOffset < bone.myChildCount;
			childOffset++)
		{
			bonesToVisit.push_front(bone.myFirstChildInd + childOffset);
		}
		ASSERT_STR(bonesToVisit.size() < kBonesMemorySize, "Not enough space "
			"allocated on the stack to dirty the hierarchy! Try increasing the size!");
	}

	Bone& rootOfHierarchy = myBones[indexOfRoot];
	rootOfHierarchy.myIsWorldDirty |= aIsWorldDirty;

	myNeedsUpdate = true;
}

void Skeleton::UpdateBone(BoneIndex anIndex, const Transform& aParentWorld)
{
	Bone& bone = myBones[anIndex];
	if (bone.myIsWorldDirty)
	{
		// if the world is dirty - we have to extract local 
		// transform out of it and the parent
		bone.myLocalTransf = aParentWorld * bone.myWorldTransf.GetInverted();
		bone.myIsWorldDirty = false;
		bone.myIsLocalDirty = false; // no need to update world transform based on new local
	}
	else if (bone.myIsLocalDirty)
	{
		// if the local is dirty - all we gotta do is update 
		// the world transform based on parent
		bone.myWorldTransf = aParentWorld * bone.myLocalTransf;
		bone.myIsLocalDirty = false;
	}
}