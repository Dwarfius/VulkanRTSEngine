#pragma once

#include "../Systems/ProfilerUI.h"
#include "../Animation/Skeleton.h"
#include "../Animation/AnimationClip.h"

class PhysicsWorld;
class GameObject;
class PhysicsShapeBase;
class Transform;
class Game;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(PhysicsWorld& aWorld);

	void Update(Game& aGame, float aDeltaTime, PhysicsWorld& aWorld);

private:
	void HandleCamera(Transform& aCamTransf, float aDeltaTime);

	const float myMouseSensitivity;
	float myFlightSpeed;
	bool myDemoWindowVisible;

	std::shared_ptr<PhysicsShapeBase> myPhysShape;
	ProfilerUI myProfilerUI;

	// Testing
	void InitTestSkeleton();
	void UpdateTestSkeleton(Game& aGame, float aDeltaTime);
	void DrawBoneHierarchy();
	void DrawBoneInfo();
	Skeleton myTestSkeleton;
	Skeleton::BoneIndex mySelectedBone;
	std::vector<std::function<void(Skeleton&)>> mySkeletonAnimators;
	std::unique_ptr<AnimationClip> myTestClip;
};