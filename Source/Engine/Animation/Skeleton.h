#pragma once

#include <Core/Transform.h>

class DebugDrawer;

class Skeleton
{
public:
	using BoneIndex = uint16_t;
	static constexpr BoneIndex kInvalidIndex = std::numeric_limits<BoneIndex>::max();

private:
	// TODO: Hmmm, for uploading to GPU convenience as a single memcpy
	// need to explore splitting this
	struct Bone
	{
		Transform myWorldTransf {};
		Transform myLocalTransf {};
		Transform myInverseBindTransf {};
		Skeleton::BoneIndex myParentInd = kInvalidIndex;
		Skeleton::BoneIndex myFirstChildInd = kInvalidIndex;
		Skeleton::BoneIndex myChildCount = 0;
		Skeleton::BoneIndex myIsWorldDirty : 1; // has world been updated?
		Skeleton::BoneIndex myIsLocalDirty : 1; // has local been updated?

		Bone()
			: myIsWorldDirty(false)
			, myIsLocalDirty(false)
		{
		}

		Bone(Skeleton::BoneIndex aParentInd, const Transform& aLocalTransf)
			: myLocalTransf(aLocalTransf)
			, myParentInd(aParentInd)
			, myIsLocalDirty(true)
			, myIsWorldDirty(false)
		{
		}
	};

public:
	Skeleton(BoneIndex aCapacity);

	BoneIndex GetBoneCount() const { return static_cast<BoneIndex>(myBones.size()); }
	void AddBone(BoneIndex aParentIndex, const Transform& aLocalTransf);
	BoneIndex GetParentIndex(BoneIndex anIndex) const { return myBones[anIndex].myParentInd; }

	const Transform& GetBoneLocalTransform(BoneIndex anIndex) const;
	const Transform& GetBoneWorldTransform(BoneIndex anIndex) const;
	const Transform& GetBoneIverseBindTransform(BoneIndex anIndex) const;
	void SetBoneLocalTransform(BoneIndex anIndex, const Transform& aTransform);
	void SetBoneWorldTransform(BoneIndex anIndex, const Transform& aTransform);

	// Must be called to propagate all the changes to the skeletal bones
	void Update();

	// Utility to visualize the skeleton
	void DebugDraw(DebugDrawer& aDrawer, const Transform& aWorldTransform) const;

private:
	void DirtyHierarchy(BoneIndex indexOfRoot, bool aIsWorldDirty);
	void UpdateBone(BoneIndex anIndex, const Transform& aParentWorld);

	// Bones hierarchy is stored in breadth-first order
	std::vector<Bone> myBones;
	bool myNeedsUpdate = false;
	bool myNeedUpdatingInverses = true;
};