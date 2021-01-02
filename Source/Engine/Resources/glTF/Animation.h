#pragma once

#include "Accessor.h"
#include "Animation/AnimationClip.h"

namespace glTF
{
	struct Animation
	{
		struct Sampler
		{
			uint32_t myInput; // accessorInd, for Time
			uint32_t myOutput; // accessorInd, for Target::Path
			AnimationClip::Interpolation myInterpolation;

			static Sampler Parse(const nlohmann::json& aJson);
		};
		
		struct Channel
		{
			struct Target
			{
				int myNode; // if -1, channel should be ignored
				AnimationClip::Property myPath;

				static Target Parse(const nlohmann::json& aJson);
			};

			uint32_t mySampler;
			Target myTarget;

			static Channel Parse(const nlohmann::json& aJson);

			// Inputs required to construct a Track and Marks for the AnimationClip from a channel
			struct TrackInputs : BufferAccessorInputs
			{
				const std::vector<Sampler>& mySamplers;
				const std::unordered_map<uint32_t, AnimationClip::BoneIndex>& myIndexMap;
			};
			void ConstructTrack(const TrackInputs& aInputs, Handle<AnimationClip>& aClip) const;
		};

		std::vector<Sampler> mySamplers;
		std::vector<Channel> myChannels;
		
		static std::vector<Animation> Parse(const nlohmann::json& aRootJson);

		struct AnimationClipInput : BufferAccessorInputs
		{
			const std::vector<Animation>& myAnimations;
			const std::unordered_map<uint32_t, AnimationClip::BoneIndex>& myIndexMap;
		};
		static void ConstructAnimationClips(const AnimationClipInput& aInputs, std::vector<Handle<AnimationClip>>& aClips);
	};
}