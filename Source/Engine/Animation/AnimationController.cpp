#include "../Precomp.h"
#include "AnimationController.h"

#include "Animation/Skeleton.h"

void AnimationController::PlayClip(AnimationClip* aClip)
{
	ASSERT_STR(aClip && aClip->GetLength() > 0.f, "Invalid clip passed in!");
	ASSERT_STR(aClip->GetTracks().size(), "Empty clip passed in!");
	myActiveClip = aClip;
	myCurrentTime = 0;

	InitMarks();
}

void AnimationController::Update(Skeleton& aSkeleton, float aDeltaTime)
{
	ASSERT_STR(myActiveClip, "Nothing to update!");
	ASSERT_STR(myCurrentMarks.size() >= myActiveClip->GetTracks().size(),
		"Missing current marks - did you forget to call InitMarks()?");

	myCurrentTime += aDeltaTime;
	if (myCurrentTime > myActiveClip->GetLength())
	{
		myCurrentTime -= myActiveClip->GetLength();
		if (!myActiveClip->IsLooping())
		{
			myActiveClip = nullptr;
			return;
		}
		else
		{
			InitMarks();
		}
	}

	Skeleton::BoneIndex boneIndex;
	glm::vec3 bonePos;
	glm::vec3 boneScale;
	glm::vec3 boneEulerRot;

	auto FetchActiveBone = [&](Skeleton::BoneIndex anIndex) {
		boneIndex = anIndex;
		Transform boneTransf = aSkeleton.GetBoneLocalTransform(boneIndex);
		bonePos = boneTransf.GetPos();
		boneScale = boneTransf.GetScale();
		boneEulerRot = boneTransf.GetEuler();
	};

	const std::vector<AnimationClip::BoneTrack>& tracks = myActiveClip->GetTracks();
	FetchActiveBone(tracks[0].myBone);
	size_t markIndex = 0;
	for (const AnimationClip::BoneTrack& track : tracks)
	{
		if (track.myBone != boneIndex)
		{
			// update bone
			aSkeleton.SetBoneLocalTransform(boneIndex, { bonePos, boneEulerRot, boneScale });

			// fetch new bone
			FetchActiveBone(track.myBone);
		}

		size_t currMarkInd = myCurrentMarks[markIndex];
		AnimationClip::Mark currentMark = myActiveClip->GetMark(currMarkInd);
		// it is possible that we already progressed to the next mark
		// so have to check the current time
		if (currMarkInd < track.myTrackStart + track.myMarkCount - 1)
		{
			size_t nextMarkInd = currMarkInd + 1;
			AnimationClip::Mark nextMark = myActiveClip->GetMark(nextMarkInd);
			if (myCurrentTime >= nextMark.myTimeStamp)
			{
				currentMark = nextMark;
				currMarkInd = nextMarkInd;
				// save it for later to continue from it!
				myCurrentMarks[markIndex] = nextMarkInd;
			}
		}
		float newValue = myActiveClip->CalculateValue(currMarkInd, myCurrentTime, track);
		switch (track.myAffectedProperty)
		{
		case AnimationClip::Property::PosX: bonePos.x = newValue; break;
		case AnimationClip::Property::PosY: bonePos.y = newValue; break;
		case AnimationClip::Property::PosZ: bonePos.z = newValue; break;
		case AnimationClip::Property::ScaleX: boneScale.x = newValue; break;
		case AnimationClip::Property::ScaleY: boneScale.y = newValue; break;
		case AnimationClip::Property::ScaleZ: boneScale.z = newValue; break;
		case AnimationClip::Property::RotX: boneEulerRot.x = newValue; break;
		case AnimationClip::Property::RotY: boneEulerRot.y = newValue; break;
		case AnimationClip::Property::RotZ: boneEulerRot.z = newValue; break;
		default:
			ASSERT(false);
		}
		markIndex++;
	}
	aSkeleton.SetBoneLocalTransform(boneIndex, { bonePos, boneEulerRot, boneScale });
}

void AnimationController::InitMarks()
{
	ASSERT_STR(myActiveClip, "There must be an active clip!");
	ASSERT_STR(myCurrentTime < myActiveClip->GetLength(), "Current time should be less than "
		"animation clip length, or we'll fail to find valid initial marks!");

	const std::vector<AnimationClip::BoneTrack>& tracks = myActiveClip->GetTracks();
	if (tracks.size() > myCurrentMarks.size())
	{
		myCurrentMarks.resize(tracks.size());
	}

	size_t index = 0;
	for (const AnimationClip::BoneTrack& track : tracks)
	{
		size_t markIndex = track.myTrackStart;
		while (markIndex < track.myTrackStart + track.myMarkCount
			&& myCurrentTime >= myActiveClip->GetMark(markIndex).myTimeStamp)
		{
			markIndex++;
		}
		myCurrentMarks[index++] = markIndex - 1;
	}
}