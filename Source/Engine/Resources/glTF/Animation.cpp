#include "Precomp.h"
#include "Animation.h"

namespace glTF
{
	Animation::Sampler Animation::Sampler::Parse(const nlohmann::json& aJson)
	{
		uint32_t input = aJson["input"].get<uint32_t>();
		uint32_t output = aJson["output"].get<uint32_t>();
		std::string interpolationStr = ReadOptional(aJson, "interpolation", std::string());
		AnimationClip::Interpolation interpolation = AnimationClip::Interpolation::Step;
		if (interpolationStr == "LINEAR")
		{
			interpolation = AnimationClip::Interpolation::Linear;
		}
		else if (interpolationStr == "CUBICSPLINE")
		{
			interpolation = AnimationClip::Interpolation::Cubic;
		}
		return { input, output, interpolation };
	}

	Animation::Channel::Target Animation::Channel::Target::Parse(const nlohmann::json& aJson)
	{
		int node = ReadOptional(aJson, "node", -1);
		std::string pathStr = aJson["path"].get<std::string>();
		AnimationClip::Property path = AnimationClip::Property::Position;
		if (pathStr == "rotation")
		{
			path = AnimationClip::Property::Rotation;
		}
		else if (pathStr == "scale")
		{
			path = AnimationClip::Property::Scale;
		}
		else if (pathStr == "weights")
		{
			path = AnimationClip::Property::Weights;
		}
		return { node, path };
	}

	Animation::Channel Animation::Channel::Parse(const nlohmann::json& aJson)
	{
		uint32_t sampler = aJson["sampler"].get<uint32_t>();
		Target target = Target::Parse(aJson["target"]);
		return { sampler, target };
	}

	void Animation::Channel::ConstructTrack(const TrackInputs& aInputs, Handle<AnimationClip>& aClip) const
	{
		ASSERT_STR(myTarget.myNode > -1, "Invalid Channel, should've been skipped!");
		if (myTarget.myPath == AnimationClip::Property::Weights)
		{
			ASSERT_STR(false, "Weigths are unsupported!");
			return;
		}

		const Sampler& sampler = aInputs.mySamplers[mySampler];

		const Accessor& timeAccessor = aInputs.myAccessors[sampler.myInput];
		const Accessor& valueAccessor = aInputs.myAccessors[sampler.myOutput];
		ASSERT_STR(timeAccessor.myCount == valueAccessor.myCount, "Weird, for every time stamp there should be a value!");

		const std::vector<BufferView>& bufferViews = aInputs.myBufferViews;
		const std::vector<Buffer>& buffers = aInputs.myBuffers;

		std::vector<AnimationClip::Mark> marks;
		// over-reserve to avoid an extra realloc if we have t0 mark missing
		marks.reserve(timeAccessor.myCount + 1);
		for (size_t index = 0; index < timeAccessor.myCount; index++)
		{
			float time;
			timeAccessor.ReadElem(time, index, bufferViews, buffers);

			// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0/#animation-samplerinput-white_check_mark
			// glTF 2.0 permits recorded marks to start with t>=0, but we expect to always
			// have a t == 0 for beginning in AnimationController logic
			// (otherwise, the logic to handle it will be messy)
			const bool mustInjectStartMark = index == 0 && time > 0.f;

			switch (myTarget.myPath)
			{
			case AnimationClip::Property::Position:
			{
				glm::vec3 pos;
				valueAccessor.ReadElem(pos, index, bufferViews, buffers);
				// TODO: [[unlikely]]
				if (mustInjectStartMark)
				{
					marks.emplace_back(0.f, aInputs.myNodes[myTarget.myNode].myTransform.GetPos());
				}
				marks.emplace_back(time, pos);
			}
			break;
			case AnimationClip::Property::Rotation:
			{
				glm::quat rot;
				valueAccessor.ReadElem(rot, index, bufferViews, buffers);
				// TODO: [[unlikely]]
				if (mustInjectStartMark)
				{
					marks.emplace_back(0.f, aInputs.myNodes[myTarget.myNode].myTransform.GetRotation());
				}
				marks.emplace_back(time, rot);
			}
			break;
			case AnimationClip::Property::Scale:
			{
				glm::vec3 scale;
				valueAccessor.ReadElem(scale, index, bufferViews, buffers);
				// TODO: [[unlikely]]
				if (mustInjectStartMark)
				{
					marks.emplace_back(0.f, aInputs.myNodes[myTarget.myNode].myTransform.GetScale());
				}
				marks.emplace_back(time, scale);
			}
			break;
			default:
				ASSERT(false);
			}
		}
		
		uint32_t nodeIndex = static_cast<uint32_t>(myTarget.myNode);
		// we have to remap the index, as we store the bones in different hierarchy
		// (not as part of Node tree, but as a seperate Skeleton tree)
		AnimationClip::BoneIndex boneIndex = aInputs.myIndexMap.at(nodeIndex);
		AnimationClip::Interpolation interpolation = sampler.myInterpolation;
		aClip->AddTrack(boneIndex, myTarget.myPath, interpolation, marks);
	}

	std::vector<Animation> Animation::Parse(const nlohmann::json& aRootJson)
	{
		std::vector<Animation> animations;
		const auto& animationsJsonIter = aRootJson.find("animations");
		if (animationsJsonIter == aRootJson.end())
		{
			return animations;
		}

		animations.reserve(animationsJsonIter->size());
		for (const auto& animationJson : *animationsJsonIter)
		{
			Animation animation;

			animation.myName = ReadOptional(animationJson, "name", std::string{});

			const nlohmann::json& samplersJson = animationJson["samplers"];
			for (const nlohmann::json& samplerJson : samplersJson)
			{
				Sampler sampler = Sampler::Parse(samplerJson);
				animation.mySamplers.emplace_back(std::move(sampler));
			}

			const nlohmann::json& channelsJson = animationJson["channels"];
			for (const nlohmann::json& channelJson : channelsJson)
			{
				Channel channel = Channel::Parse(channelJson);
				ASSERT_STR(channel.myTarget.myNode > -1, "Well, didn't think there would be such "
					"a channel - it should be discarded!");
				animation.myChannels.emplace_back(std::move(channel));
			}

			animations.push_back(std::move(animation));
		}

		return animations;
	}

	void Animation::ConstructAnimationClips(const AnimationClipInput& aInputs, 
		std::vector<Handle<AnimationClip>>& aClips, 
		std::vector<std::string>& aNames)
	{
		for (const Animation& animation : aInputs.myAnimations)
		{
			float length = 0;

			// first identify the length of animation
			for (const Sampler& sampler : animation.mySamplers)
			{
				const Accessor& timeAccessor = aInputs.myAccessors[sampler.myInput];
				ASSERT_STR(timeAccessor.myType == Accessor::Type::Scalar,
					"Time accessor must be Scalar!");

				// According to docs, for time accessor min/max must be defined!
				float newLength = 0;
				// TODO: C++20 - replace with std::bit_cast
				std::memcpy(&newLength, &timeAccessor.myMax[0], sizeof(float));
				length = std::max(length, newLength);
			}
			ASSERT_STR(length > 0, "Failed to find the length of animation!");

			// now we can start populating the clip
			Handle<AnimationClip> clip = new AnimationClip(length, true);
			for (const Channel& channel : animation.myChannels)
			{
				Channel::TrackInputs input
				{
					aInputs.myBuffers,
					aInputs.myBufferViews,
					aInputs.myAccessors,
					aInputs.myNodes,
					animation.mySamplers,
					aInputs.myIndexMap
				};
				channel.ConstructTrack(input, clip);
			}
			aClips.push_back(std::move(clip));
			aNames.push_back(animation.myName);
		}
	}
}