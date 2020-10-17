#include "Precomp.h"
#include "AnimationClip.h"

#include "Animation/Skeleton.h"

static_assert(std::is_same_v<AnimationClip::BoneIndex, Skeleton::BoneIndex>, "Bone Indices must match!");

AnimationClip::AnimationClip(float aLength, bool aIsLooping)
	: myLength(aLength)
	, myIsLooping(aIsLooping)
{
}

void AnimationClip::AddTrack(BoneIndex anIndex, Property aProperty, Interpolation anInterMode, const std::vector<Mark>& aMarkSet)
{
	// In order to make it easier to animate bones
	// we need to order both the Marks and BoneTracks
	std::vector<BoneTrack>::iterator insertIter = myTracks.begin();
	while (insertIter != myTracks.end()
		&& insertIter->myBone < anIndex)
	{
		insertIter++;
	}

	size_t trackStart = 0;
	if (insertIter != myTracks.end())
	{
		trackStart = insertIter->myTrackStart;
	}
	// need to cache the iterator offset for refreshing later
	const size_t newTrackOffset = insertIter - myTracks.begin();
	myTracks.insert(insertIter, { trackStart, aMarkSet.size(), anIndex, aProperty, anInterMode });
	myMarks.insert(myMarks.begin() + trackStart, aMarkSet.begin(), aMarkSet.end());

	// now, all that's left is to offset the BoneTrack starts!
	// have to refresh the iterator after insertion
	insertIter = myTracks.begin() + newTrackOffset;
	// starting past inserted
	while (++insertIter != myTracks.end())
	{
		insertIter->myTrackStart += aMarkSet.size();
	}
}

float AnimationClip::CalculateValue(size_t aMarkInd, float aTime, const BoneTrack& aTrack) const
{
	ASSERT_STR(aMarkInd < myMarks.size(), "Invalid mark index!");
	ASSERT_STR(aMarkInd >= aTrack.myTrackStart 
		&& aMarkInd < aTrack.myTrackStart + aTrack.myMarkCount,
		"This mark doesn't bellong to the bone track!");
	ASSERT_STR(aTime >= 0 && aTime <= myLength, "Time outside of clips's length!");
	const bool isLastMark = aMarkInd + 1 == aTrack.myTrackStart + aTrack.myMarkCount;
	const Mark fromMark = myMarks[aMarkInd];
	const size_t toMarkInd = isLastMark ? aMarkInd : (aMarkInd + 1);
	const Mark toMark = myMarks[toMarkInd];
	ASSERT_STR((aTime >= fromMark.myTimeStamp && aTime <= toMark.myTimeStamp) || isLastMark,
		"Invalid aMarkInd passed in for aTime!");
	const float relFactor = isLastMark ? 0 : ((aTime - fromMark.myTimeStamp) / (toMark.myTimeStamp - fromMark.myTimeStamp));
	float result = 0;
	switch (aTrack.myInterpolation)
	{
	case Interpolation::Linear:
		result = fromMark.myValue + relFactor * (toMark.myValue - fromMark.myValue);
		break;
	default:
		ASSERT(false);
	}
	return result;
}