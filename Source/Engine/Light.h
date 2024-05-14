#pragma once

#include <Core/DataEnum.h>
#include <Core/Pool.h>
#include <Core/Transform.h>

#include <Graphics/Descriptor.h>
#include <Graphics/UniformAdapterRegister.h>

struct Light
{
	DATA_ENUM(Type, char,
		Point,
		Spot,
		Directional
	);

	Transform myTransform;
	glm::vec3 myColor;
	float myAmbientIntensity = 0.05f;
	glm::vec3 myAttenuation = {1.f, 0.09f, 0.03f};
	glm::vec2 mySpotlightLimits = { 0.95f, 0.9f };
	Type myType;
};

class LightSystem
{
public:
	template<class T>
	using Ptr = PoolPtr<T>;

	Ptr<Light> AllocateLight() { return myLights.Allocate(); }

	template<class T>
	void ForEach(const T& aFunc) { myLights.ForEach(aFunc); }
	template<class T>
	void ForEach(const T& aFunc) const { myLights.ForEach(aFunc); }

	size_t GetCount() const { return myLights.GetSize(); }

private:
	Pool<Light> myLights;
};

class LightAdapter : RegisterUniformAdapter<LightAdapter, true>
{
public:
	constexpr static uint32_t kMaxLights = 64;
	constexpr static Descriptor ourDescriptor{
		{ Descriptor::UniformType::Vec4, kMaxLights }, // pos + ambient intensity
		{ Descriptor::UniformType::Vec4, kMaxLights }, // light dir + spot inner limit
		{ Descriptor::UniformType::Vec4, kMaxLights }, // color + type
		{ Descriptor::UniformType::Vec4, kMaxLights }, // attenuation + spot outer limit
		{ Descriptor::UniformType::Int } // count
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};