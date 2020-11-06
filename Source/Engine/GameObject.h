#pragma once

#include <Graphics/Graphics.h>
#include <Core/Transform.h>
#include <Core/UID.h>
#include <Physics/PhysicsEntity.h>
#include <Core/Pool.h>

class VisualObject;
class ComponentBase;
class Skeleton;
class AnimationController;

class GameObject : public IPhysControllable
{
public:
	GameObject(glm::vec3 aPos, glm::vec3 aRot, glm::vec3 aScale);
	~GameObject();

	void Update(float aDeltaTime);

	Transform& GetTransform() { return myTransf; }
	const Transform& GetTransform() const { return myTransf; }
	const glm::mat4& GetMatrix() const { return myTransf.GetMatrix(); }

	const UID& GetUID() const { return myUID; }

	template<class TComp, class... TArgs>
	TComp* AddComponent(TArgs&&... anArgs);

	template<class TComp>
	TComp* GetComponent() const;

	void SetVisualObject(VisualObject* aVisualObject) { myVisualObject = aVisualObject; }
	const VisualObject* GetVisualObject() const { return myVisualObject; }
	VisualObject* GetVisualObject() { return myVisualObject; }

	bool IsDead() const { return myIsDead; }
	void Die();

	const PoolPtr<Skeleton>& GetSkeleton() const { return mySkeleton; }
	void SetSkeleton(PoolPtr<Skeleton>&& aSkeleton) { mySkeleton = std::move(aSkeleton); }

	const PoolPtr<AnimationController>& GetAnimController() const {	return myAnimController; }
	void SetAnimController(PoolPtr<AnimationController>&& aController) { myAnimController = std::move(aController); }

	// IPhysControllable impl
private:
	virtual void SetTransformImpl(const glm::mat4& aTransf) override final;
	virtual void GetTransformImpl(glm::mat4& aTranfs) const override final;

private:
	UID myUID;
	
	Transform myTransf;
	glm::vec3 myCenter;

	bool myIsDead;

	std::vector<ComponentBase*> myComponents;
	VisualObject* myVisualObject;
	PoolPtr<Skeleton> mySkeleton;
	PoolPtr<AnimationController> myAnimController;
};

template<class TComp, class... TArgs>
TComp* GameObject::AddComponent(TArgs&&... anArgs)
{
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