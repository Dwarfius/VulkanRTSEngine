#include "../Precomp.h"
#include "AnimationController.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimationClip.h"

AnimationController::AnimationController(const PoolWeakPtr<Skeleton>& aSkeleton)
	: mySkeleton(aSkeleton)
{
}

void AnimationController::PlayClip(const AnimationClip* aClip)
{
	ASSERT_STR(aClip && aClip->GetLength() > 0.f, "Invalid clip passed in!");
	ASSERT_STR(aClip->GetTracks().size(), "Empty clip passed in!");
	myActiveClip = aClip;
	myCurrentTime = 0;

	InitMarks();
}

void AnimationController::Update(float aDeltaTime)
{
	ASSERT_STR(mySkeleton.IsValid(), "Attempt to update skeleton that has been released!");
	ASSERT_STR(myActiveClip, "Nothing to update!");
	ASSERT_STR(myCurrentMarks.size() >= myActiveClip->GetTracks().size(),
		"Missing current marks - did you forget to call InitMarks()?");

	Skeleton* skeleton = mySkeleton.Get();

	myCurrentTime += aDeltaTime;
	if (myCurrentTime > myActiveClip->GetLength())
	{
		myCurrentTime = glm::mod(myCurrentTime, myActiveClip->GetLength());
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
	glm::quat boneRot;

	auto FetchActiveBone = [&](Skeleton::BoneIndex anIndex) {
		boneIndex = anIndex;
		Transform boneTransf = skeleton->GetBoneLocalTransform(boneIndex);
		bonePos = boneTransf.GetPos();
		boneScale = boneTransf.GetScale();
		boneRot = boneTransf.GetRotation();
	};

	const std::vector<AnimationClip::BoneTrack>& tracks = myActiveClip->GetTracks();
	FetchActiveBone(tracks[0].myBone);
	for(size_t trackIndex = 0; trackIndex < tracks.size(); trackIndex++)
	{
		const AnimationClip::BoneTrack& track = tracks[trackIndex];
		if (track.myBone != boneIndex)
		{
			ASSERT_STR(track.myBone > boneIndex, "Bad ordering!");
			// update bone
			skeleton->SetBoneLocalTransform(boneIndex, { bonePos, boneRot, boneScale });

			// fetch new bone
			FetchActiveBone(track.myBone);
		}

		size_t currMarkInd = myCurrentMarks[trackIndex];
		AnimationClip::Mark currentMark = myActiveClip->GetMark(currMarkInd);

		// TODO: take this out of the loop and put above
		// find the smallest deltaTime of mark-pair, and hide this behind the check
		{
			// it is possible that we already progressed to the next mark
			// (or more, if the framerate is low) so have to update where we are
			// against current time
			size_t nextMarkInd = currMarkInd + 1;
			const size_t trackEnd = track.myTrackStart + track.myMarkCount;
			for(; nextMarkInd < trackEnd; nextMarkInd++)
			{
				AnimationClip::Mark nextMark = myActiveClip->GetMark(nextMarkInd);
				if (myCurrentTime >= nextMark.myTimeStamp)
				{
					currentMark = nextMark;
					currMarkInd = nextMarkInd;
					// save it for later to continue from it!
					myCurrentMarks[trackIndex] = nextMarkInd;
				}
				else
				{
					break;
				}
			}
		}

		switch (track.myAffectedProperty)
		{
		case AnimationClip::Property::Position: 
			bonePos = myActiveClip->CalculateVec(currMarkInd, myCurrentTime, track);
			break;
		case AnimationClip::Property::Rotation:
			boneRot = myActiveClip->CalculateQuat(currMarkInd, myCurrentTime, track);
			break;
		case AnimationClip::Property::Scale: 
			boneScale = myActiveClip->CalculateVec(currMarkInd, myCurrentTime, track);
			break;
		default:
			ASSERT(false);
		}
	}
	skeleton->SetBoneLocalTransform(boneIndex, { bonePos, boneRot, boneScale });
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
		ASSERT_STR(myActiveClip->GetMark(track.myTrackStart).myTimeStamp == 0.f,
			"We require that there is a mark at t==0 bellow and in ::Update!");
		size_t markIndex = track.myTrackStart;
		while (markIndex < track.myTrackStart + track.myMarkCount
			&& myCurrentTime >= myActiveClip->GetMark(markIndex).myTimeStamp)
		{
			markIndex++;
		}
		myCurrentMarks[index++] = markIndex - 1;
	}
}