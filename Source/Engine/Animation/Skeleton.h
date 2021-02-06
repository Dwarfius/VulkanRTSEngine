#pragma once

#include <Core/Transform.h>

class DebugDrawer;

class Skeleton
{
public:
	using BoneIndex = uint16_t;
	static constexpr BoneIndex kInvalidIndex = std::numeric_limits<BoneIndex>::max();

	class BoneInitData
	{
		friend class Skeleton;
		Transform myLocalTransf;
		Skeleton::BoneIndex myParentInd;

	public:
		BoneInitData(const Transform& aLocalTransf, Skeleton::BoneIndex aParentInd)
			: myLocalTransf(aLocalTransf)
			, myParentInd(aParentInd)
		{
		}
	};

private:
	struct Bone
	{
		Transform myWorldTransf {};
		Transform myLocalTransf {};
		glm::mat4 myInverseBindTransf = glm::mat4(1.f);
		Skeleton::BoneIndex myParentInd = kInvalidIndex;

		Bone() = default;

		Bone(Skeleton::BoneIndex aParentInd, const Transform& aLocalTransf)
			: myLocalTransf(aLocalTransf)
			, myParentInd(aParentInd)
		{
		}
	};

public:
	Skeleton(BoneIndex aCapacity);
	Skeleton(const std::vector<BoneInitData>& aBones);

	BoneIndex GetBoneCount() const { return static_cast<BoneIndex>(myBones.size()); }
	void AddBone(BoneIndex aParentIndex, const Transform& aLocalTransf);
	BoneIndex GetParentIndex(BoneIndex anIndex) const { return myBones[anIndex].myParentInd; }

	const Transform& GetBoneLocalTransform(BoneIndex anIndex) const;
	Transform GetBoneWorldTransform(BoneIndex anIndex) const;
	const glm::mat4& GetBoneIverseBindTransform(BoneIndex anIndex) const;
	void SetBoneLocalTransform(BoneIndex anIndex, const Transform& aTransform, bool aUpdateHierarchy = true);
	void SetBoneWorldTransform(BoneIndex anIndex, const Transform& aTransform, bool aUpdateHierarchy = true);
	void OverrideInverseBindTransform(BoneIndex anIndex, const glm::mat4& anInverseMat);

	// Utility to visualize the skeleton
	void DebugDraw(DebugDrawer& aDrawer, const Transform& aWorldTransform) const;

private:
	void UpdateHierarchy(BoneIndex anIndex);

	// Bones hierarchy is stored in breadth-first order
	std::vector<Bone> myBones;
};