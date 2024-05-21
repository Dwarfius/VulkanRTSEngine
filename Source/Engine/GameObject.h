#pragma once

#include <Graphics/Graphics.h>
#include <Core/Transform.h>
#include <Core/UID.h>
#include <Physics/PhysicsEntity.h>
#include <Core/Pool.h>
#include <Core/Resources/Resource.h>

#include "Animation/AnimationController.h"
#include "VisualObject.h"

class ComponentBase;
class GameObject;
class World;

struct Renderable
{
	VisualObject myVO;
	GameObject* myGO; // non-owning, non-null
};

class GameObject : public Resource, public IPhysControllable
{
public:
	constexpr static StaticString kExtension = ".go";

	GameObject(const Transform& aTransform);
	GameObject(Id anId, std::string_view aPath);
	~GameObject();

	const Transform& GetLocalTransform() const { return myLocalTransf; }
	const Transform& GetWorldTransform() const { return myWorldTransf; }
	void SetLocalTransform(const Transform& aTransf, bool aMoveChildren = true);
	void SetWorldTransform(const Transform& aTransf, bool aMoveChildren = true);

	const UID& GetUID() const { return myUID; }
	
	template<class TComp, class... TArgs>
	TComp* AddComponent(TArgs&&... anArgs);

	template<class TComp>
	TComp* GetComponent() const;

	void CreateRenderable();
	const Renderable* GetRenderable() const { return myRenderable; }
	Renderable* GetRenderable() { return myRenderable; }

	bool IsDead() const { return myIsDead; }
	void Die();

	const PoolPtr<Skeleton>& GetSkeleton() const { return mySkeleton; }
	void SetSkeleton(PoolPtr<Skeleton>&& aSkeleton) { mySkeleton = std::move(aSkeleton); }

	const PoolPtr<AnimationController>& GetAnimController() const {	return myAnimController; }
	void SetAnimController(PoolPtr<AnimationController>&& aController) { myAnimController = std::move(aController); }

	Handle<GameObject> GetParent() const { return myParent; }
	void SetParent(Handle<GameObject>& aParent, bool aKeepWorldTransf = true);
	void DetachFromParent();
	
	size_t GetChildCount() const { return myChildren.size(); }
	const GameObject& GetChild(size_t aInd) const { return *myChildren[aInd].Get(); }
	GameObject& GetChild(size_t aInd) { return *myChildren[aInd].Get(); }
	void AddChild(Handle<GameObject>& aChild, bool aKeepChildWorldTransf = true);
	void DetachChild(size_t aInd);
	void DetachChild(const Handle<GameObject>& aGO);

	void SetWorld(World* aWorld);
	World* GetWorld() { return myWorld; }
	const World* GetWorld() const { return myWorld; }

	void Serialize(Serializer& aSerializer) final;

	// IPhysControllable impl
private:
	void SetPhysTransform(const glm::mat4& aTransf) override final;
	void GetPhysTransform(glm::mat4& aTranfs) const override final;

private:
	friend class HierarchyAccess;
	void UpdateHierarchyTransform();

	UID myUID;
	
	Transform myWorldTransf;
	Transform myLocalTransf;
	glm::vec3 myCenter;

	std::vector<ComponentBase*> myComponents;
	Renderable* myRenderable = nullptr; // owning
	PoolPtr<Skeleton> mySkeleton;
	PoolPtr<AnimationController> myAnimController;
	
	Handle<GameObject> myParent;
	std::vector<Handle<GameObject>> myChildren;
	bool myIsDead;

	World* myWorld = nullptr;

#ifdef ASSERT_MUTEX
	AssertMutex myPhysMutex;
#endif
};

template<class TComp, class... TArgs>
TComp* GameObject::AddComponent(TArgs&&... anArgs)
{
	// TODO: get rid of this, replace with Component Pool use
	TComp* newComp = new TComp(std::forward<TArgs>(anArgs)...);
	newComp->Init(this);
	myComponents.push_back(newComp);
	return newComp;
}

template<class TComp>
TComp* GameObject::GetComponent() const
{
	for (ComponentBase* comp : myComponents)
	{
		if (TComp* dynComp = dynamic_cast<TComp*>(comp))
		{
			return dynComp;
		}
	}
	return nullptr;
}