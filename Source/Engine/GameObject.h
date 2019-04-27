#pragma once

#include <Core/Graphics/Graphics.h>
#include <Core/Transform.h>
#include <Core/UID.h>
#include <Physics/PhysicsEntity.h>

class VisualObject;
class ComponentBase;

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

	void AddComponent(ComponentBase* aComponent);
	ComponentBase* GetComponent(int aType) const;

	void SetVisualObject(VisualObject* aVisualObject) { myVisualObject = aVisualObject; }
	const VisualObject* GetVisualObject() const { return myVisualObject; }
	VisualObject* GetVisualObject() { return myVisualObject; }

	bool IsDead() const { return myIsDead; }
	void Die();

	// IPhysControllable impl
private:
	virtual void SetTransformImpl(const glm::mat4& aTransf) override final;
	virtual void GetTransformImpl(glm::mat4& aTranfs) const override final;

private:
	UID myUID;
	
	Transform myTransf;
	glm::vec3 myCenter;

	bool myIsDead;

	vector<ComponentBase*> myComponents;
	VisualObject* myVisualObject;
};