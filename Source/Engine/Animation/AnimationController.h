#pragma once

#include <Core/Pool.h>
#include "Animation/Skeleton.h"

class AnimationClip;

class AnimationController
{
public:
	AnimationController(const PoolWeakPtr<Skeleton>& aSkeleton);

	void AddClip(AnimationClip* aClip) { myClips.push_back(aClip); }
	void PlayClip(AnimationClip* aClip);

	bool NeedsUpdate() const { return myActiveClip != nullptr; }
	void Update(float aDeltaTime);

	float GetTime() const { return myCurrentTime; }

private:
	void InitMarks();

	PoolWeakPtr<Skeleton> mySkeleton;
	AnimationClip* myActiveClip = nullptr;
	std::vector<AnimationClip*> myClips;

	std::vector<size_t> myCurrentMarks;
	float myCurrentTime = 0;
};