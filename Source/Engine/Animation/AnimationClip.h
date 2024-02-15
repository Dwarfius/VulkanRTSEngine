#pragma once

#include <Core/Resources/Resource.h>
#include <Core/DataEnum.h>

class AnimationClip : public Resource
{
public:
	constexpr static StaticString kExtension = ".animclip";

	using BoneIndex = uint16_t; // must be in sync with Skeleton::BoneIndex!

	DATA_ENUM(Property, uint8_t,
		Position,
		Rotation,
		Scale,
		Weights
	);

	// http://paulbourke.net/miscellaneous/interpolation/
	DATA_ENUM(Interpolation, uint8_t,
		Step,
		Linear,
		Cubic
	);

	struct Mark
	{
		float myTimeStamp;
		glm::quat myValue;

		constexpr Mark()
			: myTimeStamp(0)
			, myValue()
		{
		}

		constexpr Mark(float aTime, glm::quat aQuat)
			: myTimeStamp(aTime)
			, myValue(aQuat)
		{
		}

		constexpr Mark(float aTime, glm::vec3 aPos)
			: myTimeStamp(aTime)
			, myValue(0, aPos.x, aPos.y, aPos.z)
		{
		}

		void Serialize(Serializer& aSerializer);
	};

	struct BoneTrack
	{
		size_t myTrackStart;
		size_t myMarkCount;
		BoneIndex myBone;
		Property myAffectedProperty;
		Interpolation myInterpolation;

		void Serialize(Serializer& aSerializer);
	};

public:
	AnimationClip(float aLength, bool myIsLooping);
	AnimationClip(Id anId, std::string_view aPath);

	bool IsLooping() const { return myIsLooping; }
	void SetLooping(bool aIsLooping) { myIsLooping = aIsLooping; }

	float GetLength() const { return myLength; }

	void AddTrack(BoneIndex anIndex, Property aProperty, Interpolation anInterMode, const std::vector<Mark>& aMarkSet);
	glm::vec3 CalculateVec(size_t aMarkInd, float aTime, const BoneTrack& aTrack) const;
	glm::quat CalculateQuat(size_t aMarkInd, float aTime, const BoneTrack& aTrack) const;

	// returns bone-ordered list of tracks
	const std::vector<BoneTrack>& GetTracks() const { return myTracks; }
	Mark GetMark(size_t anIndex) const { return myMarks[anIndex]; }

private:
	void Serialize(Serializer& aSerializer) final;

	// TODO: bench merging as a single allocation
	std::vector<BoneTrack> myTracks;
	std::vector<Mark> myMarks;

	float myLength;
	bool myIsLooping;
};