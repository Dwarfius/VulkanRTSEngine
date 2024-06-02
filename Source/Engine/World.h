#pragma once

#ifdef ASSERT_MUTEX
#include <Core/Threading/AssertRWMutex.h>
#endif
#include <Core/Resources/Resource.h>

class GameObject;
class PhysicsWorld;

class World : public Resource
{
public:
	constexpr static StaticString kExtension = ".world";

	World() = default;
	World(Id anId, std::string_view aPath);
	~World() noexcept = default;

	void Add(Handle<GameObject>& aGameObject);
	void Remove(Handle<GameObject>& aGameObject);
	void Clear();
	void Reserve(size_t aSize);

	void CreatePhysWorld();
	PhysicsWorld* GetPhysicsWorld() { return myPhysWorld; }
	const PhysicsWorld* GetPhysicsWorld() const { return myPhysWorld; }

	template<class TFunc> 
		requires std::is_invocable_v<
			TFunc, std::vector<Handle<GameObject>>&
		>
	void Access(TFunc aFunc)
	{
#ifdef ASSERT_MUTEX
		AssertWriteLock lock(myObjectsMutex);
#endif
		aFunc(myGameObjects);
	}

	template<class TFunc> 
		requires std::is_invocable_v<
			TFunc, const std::vector<Handle<GameObject>>&
		>
	void Access(TFunc aFunc) const
	{
#ifdef ASSERT_MUTEX
		AssertReadLock lock(myObjectsMutex);
#endif
		aFunc(myGameObjects);
	}

	std::string_view GetTypeName() const override { return "World"; }

private:
	void Serialize(Serializer& aSerializer) final;

	// TODO: Merge into flat_map from C++23
	std::vector<Handle<GameObject>> myGameObjects;
	std::unordered_map<const GameObject*, size_t> myGameObjIndices;
	PhysicsWorld* myPhysWorld = nullptr;
#ifdef ASSERT_MUTEX
	AssertRWMutex myObjectsMutex;
#endif
};

