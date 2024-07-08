#include "Precomp.h"
#include "World.h"

#include <Physics/PhysicsWorld.h>
#include <Core/Resources/Serializer.h>

#include "GameObject.h"

World::World(Id anId, std::string_view aPath)
	: Resource(anId, aPath)
{
}

void World::Add(Handle<GameObject>& aGameObject)
{
	ASSERT_STR(aGameObject.IsValid(), "Invalid object passed in!");

#ifdef ASSERT_MUTEX
	AssertWriteLock lock(myObjectsMutex);
#endif

	GameObject* objPtr = aGameObject.Get();
	ASSERT(!myGameObjIndices.contains(objPtr));

	const size_t newIndex = myGameObjects.size();
	myGameObjects.push_back(aGameObject);
	myGameObjIndices.insert({ objPtr, newIndex });

	aGameObject->SetWorld(this);
}

void World::Remove(Handle<GameObject>& aGameObject)
{
	ASSERT_STR(aGameObject.IsValid(), "Invalid object passed in!");

#ifdef ASSERT_MUTEX
	AssertWriteLock lock(myObjectsMutex);
#endif

	GameObject* objPtr = aGameObject.Get();
	const auto objIndIter = myGameObjIndices.find(objPtr);
	ASSERT_STR(objIndIter != myGameObjIndices.end(), "Game Object didn't belong to world!");
	const size_t toDeleteInd = objIndIter->second;

	const auto lastIter = myGameObjects.end() - 1;
	if (toDeleteInd != myGameObjects.size() - 1)
	{
		const auto toDeleteIter = myGameObjects.begin() + toDeleteInd;
		std::swap(*toDeleteIter, *lastIter);
		// Update the index of what used to be last, since we just swapped
		myGameObjIndices[toDeleteIter->Get()] = toDeleteInd;
	}
	myGameObjects.erase(lastIter);
	myGameObjIndices.erase(objIndIter);

	objPtr->SetWorld(nullptr);
}

void World::Clear()
{
#ifdef ASSERT_MUTEX
	AssertWriteLock lock(myObjectsMutex);
#endif
	myGameObjects.clear();
	myGameObjIndices.clear();

	if (myPhysWorld)
	{
		delete myPhysWorld;
	}
}

void World::Reserve(size_t aSize)
{
#ifdef ASSERT_MUTEX
	AssertWriteLock lock(myObjectsMutex);
#endif
	myGameObjects.reserve(aSize);
	myGameObjIndices.reserve(aSize);
}

void World::CreatePhysWorld()
{
	ASSERT(!myPhysWorld);
	myPhysWorld = new PhysicsWorld();
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