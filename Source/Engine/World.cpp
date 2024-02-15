#include "Precomp.h"
#include "World.h"

#include <Core/Resources/Serializer.h>

#include "GameObject.h"

World::World(Id anId, std::string_view aPath)
	: Resource(anId, aPath)
{
}

void World::Add(const Handle<GameObject>& aGameObject)
{
	ASSERT_STR(aGameObject.IsValid(), "Invalid object passed in!");

#ifdef ASSERT_MUTEX
	AssertWriteLock lock(myObjectsMutex);
#endif
	myGameObjects.push_back(aGameObject);
}

void World::Remove(const Handle<GameObject>& aGameObject)
{
	ASSERT_STR(aGameObject.IsValid(), "Invalid object passed in!");

#ifdef ASSERT_MUTEX
	AssertWriteLock lock(myObjectsMutex);
#endif

	const GameObject* objPtr = aGameObject.Get();
	[[maybe_unused]] const size_t count = std::erase_if(myGameObjects, 
		[=](const Handle<GameObject>& aGo) {
			return aGo.Get() == objPtr;
		}
	);
	ASSERT_STR(count > 0, "Game Object didn't belong to world!");
}

void World::Clear()
{
#ifdef ASSERT_MUTEX
	AssertWriteLock lock(myObjectsMutex);
#endif
	myGameObjects.clear();
}

void World::Reserve(size_t aSize)
{
#ifdef ASSERT_MUTEX
	AssertWriteLock lock(myObjectsMutex);
#endif
	myGameObjects.reserve(aSize);
}

void World::Serialize(Serializer& aSerializer)
{
	[[maybe_unused]] uint8_t version = 0;
	aSerializer.SerializeVersion(version);

	if (!aSerializer.IsReading())
	{
		AssetTracker& tracker = aSerializer.GetAssetTracker();
		const std::string_view worldPath = GetPath();
		const std::string_view pathWithoutExt = worldPath.substr(0, worldPath.rfind('/'));
		const std::string pathStr(pathWithoutExt.data(), pathWithoutExt.size());
		char path[256]{};
		size_t i = 0;
		for (Handle<GameObject>& go : myGameObjects)
		{
			if (go->GetPath().empty())
			{
				Utils::StringFormat(path, "{}/GameObjects/GameObject{}", pathStr, i);
				tracker.SaveAndTrack(path, go);
				i++;
			}
		}
	}
	aSerializer.Serialize("myGameObjects", myGameObjects);
}