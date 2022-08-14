#include "Precomp.h"
#include "Serializer.h"

#include "Resource.h"
#include "AssetTracker.h"

Serializer::Serializer(AssetTracker& anAssetTracker, bool aIsReading)
	: myAssetTracker(anAssetTracker)
	, myIsReading(aIsReading)
{
}

void Serializer::SerializeVersion(uint8_t& aVersion)
{
	if (myIsReading)
	{
		// This covers cases where an upgrade has happened,
		// and we try to read a version, but there isn't one
		// which means we'll return version 1 by default
		aVersion = 1;
	}
	Serialize("version", aVersion);
}

void Serializer::SerializeResource(std::string_view aName, ResourceProxy& aResource)
{
	if (BeginSerializeObjectImpl(aName))
	{
		Serialize("myPath", aResource.myPath);
	}
	EndSerializeObjectImpl(aName);
}