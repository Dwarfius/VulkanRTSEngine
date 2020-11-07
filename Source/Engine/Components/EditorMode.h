#pragma once

#include "../Systems/ProfilerUI.h"
#include "../Animation/Skeleton.h"
#include "../Animation/AnimationController.h"
#include "../Animation/AnimationClip.h"
#include <Core/Pool.h>

class PhysicsWorld;
class GameObject;
class PhysicsShapeBase;
class Transform;
class Game;
class AnimationSystem;
class OBJImporter;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(Game& aGame);

	void Update(Game& aGame, float aDeltaTime, PhysicsWorld& aWorld);

private:
	void HandleCamera(Transform& aCamTransf, float aDeltaTime);

	const float myMouseSensitivity = 0.1f;
	float myFlightSpeed = 2.f;
	bool myDemoWindowVisible = false;

	std::shared_ptr<PhysicsShapeBase> myPhysShape;
	ProfilerUI myProfilerUI;

	// Testing
	void AddTestSkeleton(AnimationSystem& anAnimSystem);
	void UpdateTestSkeleton(Game& aGame, float aDeltaTime);
	void DrawBoneHierarchy(int aSkeletonIndex);
	void DrawBoneInfo(int aSkeletonIndex);
	
	Skeleton::BoneIndex mySelectedBone = Skeleton::kInvalidIndex;
	std::vector<PoolPtr<AnimationController>> myControllers;
	std::vector<PoolPtr<Skeleton>> mySkeletons;
	Handle<AnimationClip> myAnimClip;
	Handle<OBJImporter> myImportedCube;
	int mySelectedSkeleton = -1;
	int myAddSkeletonCount = 0;
	bool myShowSkeletonUI = false;
};