#include "../Precomp.h"
#include "AnimationSystem.h"

#include <Core/Debug/DebugDrawer.h>

AnimationSystem::Ptr<Skeleton> AnimationSystem::AllocateSkeleton(Skeleton::BoneIndex aCapacity /*= 0*/)
{
	tbb::spin_mutex::scoped_lock lock(mySkeletonMutex);
#ifdef ASSERT_MUTEX
	// it is prohibited to change pool while updating!
	AssertLock updateLock(myUpdateMutex);
#endif
	return mySkeletonPool.Allocate(aCapacity);
}

AnimationSystem::Ptr<AnimationController> AnimationSystem::AllocateController
											(const WeakPtr<Skeleton>& aSkeleton)
{
	tbb::spin_mutex::scoped_lock lock(myControllerMutex);
#ifdef ASSERT_MUTEX
	// it is prohibited to change pool while updating!
	AssertLock updateLock(myUpdateMutex);
#endif
	return myControllerPool.Allocate(aSkeleton);
}

void AnimationSystem::Update(float aDeltaTime)
{
#ifdef ASSERT_MUTEX
	// it is prohibited to change pool while updating!
	AssertLock updateLock(myUpdateMutex);
#endif

	myControllerPool.ForEach([aDeltaTime](AnimationController& aController) {
		if (aController.NeedsUpdate())
		{
			aController.Update(aDeltaTime);
		}
	});

	mySkeletonPool.ForEach([](Skeleton& aSkeleton) {
		aSkeleton.Update();
	});
}