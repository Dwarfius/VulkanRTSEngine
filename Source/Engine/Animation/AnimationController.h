#pragma once

#include "Animation/AnimationClip.h"

class Skeleton;

class AnimationController
{
public:
	void AddClip(AnimationClip* aClip) { myClips.push_back(aClip); }
	void PlayClip(AnimationClip* aClip);

	bool NeedsUpdate() const { return myActiveClip != nullptr; }
	void Update(Skeleton& aSkeleton, float aDeltaTime);

	float GetTime() const { return myCurrentTime; }

private:
	void InitMarks();

	AnimationClip* myActiveClip = nullptr;
	std::vector<AnimationClip*> myClips;

	std::vector<size_t> myCurrentMarks;
	float myCurrentTime = 0;
};