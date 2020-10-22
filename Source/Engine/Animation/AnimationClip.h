#pragma once

#include <Core/Resources/Resource.h>

// TODO: make inherit from RefCounted
class AnimationClip : public Resource
{
public:
	static constexpr StaticString kDir = Resource::AssetsFolder + "anims/";
	using BoneIndex = uint16_t; // must be in sync with Skeleton::BoneIndex!

	enum class Property : uint8_t
	{
		PosX,
		PosY,
		PosZ,
		ScaleX,
		ScaleY,
		ScaleZ,
		RotX,
		RotY,
		RotZ
	};

	// http://paulbourke.net/miscellaneous/interpolation/
	enum class Interpolation : uint8_t
	{
		Linear
		//Cubic
	};

	struct Mark
	{
		float myTimeStamp;
		float myValue;
	};

	struct BoneTrack
	{
		size_t myTrackStart;
		size_t myMarkCount;
		BoneIndex myBone;
		Property myAffectedProperty;
		Interpolation myInterpolation;
	};

public:
	AnimationClip(float aLength, bool myIsLooping);
	AnimationClip(Id anId, const std::string& aPath);

	bool IsLooping() const { return myIsLooping; }
	void SetLooping(bool aIsLooping) { myIsLooping = aIsLooping; }

	float GetLength() const { return myLength; }

	void AddTrack(BoneIndex anIndex, Property aProperty, Interpolation anInterMode, const std::vector<Mark>& aMarkSet);
	float CalculateValue(size_t aMarkInd, float aTime, const BoneTrack& aTrack) const;

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