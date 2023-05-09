#pragma once

#ifdef ASSERT_MUTEX
#include <Core/Threading/AssertRWMutex.h>
#endif
#include <Core/Resources/Resource.h>

#include "GameObject.h"

class World : public Resource
{
public:
	constexpr static StaticString kExtension = ".world";

	World() = default;
	World(Id anId, const std::string& aPath);
	~World() noexcept = default;

	void Add(const Handle<GameObject>& aGameObject);
	void Remove(const Handle<GameObject>& aGameObject);
	void Clear();
	void Reserve(size_t aSize);

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

private:
	void Serialize(Serializer& aSerializer) final;

	std::vector<Handle<GameObject>> myGameObjects;
#ifdef ASSERT_MUTEX
	AssertRWMutex myObjectsMutex;
#endif
};

