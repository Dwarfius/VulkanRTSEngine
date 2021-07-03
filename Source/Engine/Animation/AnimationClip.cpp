#include "Precomp.h"
#include "AnimationClip.h"

#include "Animation/Skeleton.h"

#include <Core/Resources/Serializer.h>

static_assert(std::is_same_v<AnimationClip::BoneIndex, Skeleton::BoneIndex>, "Bone Indices must match!");

AnimationClip::AnimationClip(float aLength, bool aIsLooping)
	: myLength(aLength)
	, myIsLooping(aIsLooping)
{
}

AnimationClip::AnimationClip(Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myLength(0.f)
	, myIsLooping(false)
{
}

void AnimationClip::AddTrack(BoneIndex anIndex, Property aProperty, Interpolation anInterMode, const std::vector<Mark>& aMarkSet)
{
	ASSERT_STR(aMarkSet[0].myTimeStamp == 0.f, 
		"A track must have a mark at the start or interpolation won't work correctly!");

	// In order to make it easier to animate bones
	// we need to order both the Marks and BoneTracks
	std::vector<BoneTrack>::iterator insertIter = myTracks.begin();
	while (insertIter != myTracks.end()
		&& insertIter->myBone < anIndex)
	{
		insertIter++;
	}

	size_t trackStart = myMarks.size(); // if we didn't find it, we add to the end
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

glm::vec3 AnimationClip::CalculateVec(size_t aMarkInd, float aTime, const BoneTrack& aTrack) const
{
	ASSERT_STR(aMarkInd < myMarks.size(), "Invalid mark index!");
	ASSERT_STR(aMarkInd >= aTrack.myTrackStart 
		&& aMarkInd < aTrack.myTrackStart + aTrack.myMarkCount,
		"This mark doesn't bellong to the bone track!");
	ASSERT_STR(aTime >= 0 && aTime <= myLength, "Time outside of clips's length!");
	const Mark fromMark = myMarks[aMarkInd];
	
	glm::vec3 result(0);
	switch (aTrack.myInterpolation)
	{
	case Interpolation::Step:
		result = glm::vec3(fromMark.myValue.x, fromMark.myValue.y, fromMark.myValue.z);
		break;
	case Interpolation::Linear:
	{
		const bool isLastMark = aMarkInd + 1 == aTrack.myTrackStart + aTrack.myMarkCount;
		const size_t toMarkInd = isLastMark ? aMarkInd : (aMarkInd + 1);
		const Mark toMark = myMarks[toMarkInd];
		ASSERT_STR((aTime >= fromMark.myTimeStamp && aTime <= toMark.myTimeStamp) || isLastMark,
			"Invalid aMarkInd passed in for aTime!");
		const float relFactor = isLastMark ? 0 : ((aTime - fromMark.myTimeStamp) / (toMark.myTimeStamp - fromMark.myTimeStamp));
		const glm::vec3 fromVec = glm::vec3(fromMark.myValue.x, fromMark.myValue.y, fromMark.myValue.z);
		const glm::vec3 toVec = glm::vec3(toMark.myValue.x, toMark.myValue.y, toMark.myValue.z);
		result = glm::mix(fromVec, toVec, relFactor);
	}
		break;
	case Interpolation::Cubic:
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
		ASSERT_STR(false, "Not yet implemented!");
		break;
	default:
		ASSERT(false);
	}
	return result;
}

glm::quat AnimationClip::CalculateQuat(size_t aMarkInd, float aTime, const BoneTrack& aTrack) const
{
	ASSERT_STR(aMarkInd < myMarks.size(), "Invalid mark index!");
	ASSERT_STR(aMarkInd >= aTrack.myTrackStart
		&& aMarkInd < aTrack.myTrackStart + aTrack.myMarkCount,
		"This mark doesn't bellong to the bone track!");
	ASSERT_STR(aTime >= 0 && aTime <= myLength, "Time outside of clips's length!");
	const Mark fromMark = myMarks[aMarkInd];

	glm::quat result(0, 0, 0, 1);
	switch (aTrack.myInterpolation)
	{
	case Interpolation::Step:
		result = fromMark.myValue;
		break;
	case Interpolation::Linear:
	{
		const bool isLastMark = aMarkInd + 1 == aTrack.myTrackStart + aTrack.myMarkCount;
		const size_t toMarkInd = isLastMark ? aMarkInd : (aMarkInd + 1);
		const Mark toMark = myMarks[toMarkInd];
		ASSERT_STR((aTime >= fromMark.myTimeStamp && aTime <= toMark.myTimeStamp) || isLastMark,
			"Invalid aMarkInd passed in for aTime!");
		const float relFactor = isLastMark ? 0 : ((aTime - fromMark.myTimeStamp) / (toMark.myTimeStamp - fromMark.myTimeStamp));
		result = glm::slerp(fromMark.myValue, toMark.myValue, relFactor);
	}
	break;
	case Interpolation::Cubic:
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
		ASSERT_STR(false, "Not yet implemented!");
		break;
	default:
		ASSERT(false);
	}
	return result;
}

void AnimationClip::Serialize(Serializer& aSerializer)
{
	if (Serializer::Scope tracksScope = aSerializer.SerializeArray("myTracks", myTracks))
	{
		for (size_t i = 0; i < myTracks.size(); i++)
		{
			if (Serializer::Scope trackScope = aSerializer.SerializeObject(i))
			{
				AnimationClip::BoneTrack& track = myTracks[i];
				aSerializer.Serialize("myTrackStart", track.myTrackStart);
				aSerializer.Serialize("myMarkCount", track.myMarkCount);
				aSerializer.Serialize("myBone", track.myBone);
				aSerializer.SerializeEnum("myAffectedProperty", track.myAffectedProperty, AnimationClip::kPropertyNames);
				aSerializer.SerializeEnum("myInterpolation", track.myInterpolation, AnimationClip::kInterpolationNames);
			}
		}
	}

	if (Serializer::Scope marksScope = aSerializer.SerializeArray("myMarks", myMarks))
	{
		for (size_t i = 0; i < myMarks.size(); i++)
		{
			if (Serializer::Scope markkScope = aSerializer.SerializeObject(i))
			{
				AnimationClip::Mark& mark = myMarks[i];
				aSerializer.Serialize("myTimeStamp", mark.myTimeStamp);
				aSerializer.Serialize("myValue", mark.myValue);
			}
		}
	}

	aSerializer.Serialize("myLength", myLength);
	aSerializer.Serialize("myIsLooping", myIsLooping);
}