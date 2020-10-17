#pragma once

#include <Core/Pool.h>
#ifdef ASSERT_MUTEX
#include <Core/Threading/AssertMutex.h>
#endif

#include "Animation/AnimationController.h"
#include "Animation/Skeleton.h"

class DebugDrawer;

class AnimationSystem
{
public:
	template<class T>
	using Ptr = PoolPtr<T>;
	template<class T>
	using WeakPtr = PoolWeakPtr<T>;

	// Thread-safe allocation of skeletons
	// NOT thread safe to do it during call to ::Update!
	Ptr<Skeleton> AllocateSkeleton(Skeleton::BoneIndex aCapacity = 0);
	// Thread-safe allocation of AnimationControllers
	// NOT thread safe to do it during call to ::Update!
	Ptr<AnimationController> AllocateController(const WeakPtr<Skeleton>& aSkeleton);

	void Update(float aDeltaTime);

private:
	Pool<AnimationController> myControllerPool;
	Pool<Skeleton> mySkeletonPool;
	tbb::spin_mutex myControllerMutex;
	tbb::spin_mutex mySkeletonMutex;
	bool myIsDebugDrawingEnabled = false;
#ifdef ASSERT_MUTEX
	AssertMutex myUpdateMutex;
#endif
};