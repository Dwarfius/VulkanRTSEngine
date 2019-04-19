#pragma once

#include <Core/Graphics/Graphics.h>
#include <Core/Transform.h>
#include <Core/UID.h>

class VisualObject;
class ComponentBase;

class GameObject
{
public:
	GameObject(glm::vec3 aPos, glm::vec3 aRot, glm::vec3 aScale);
	~GameObject();

	void Update(float aDeltaTime);

	Transform& GetTransform() { return myTransf; }
	const Transform& GetTransform() const { return myTransf; }
	const glm::mat4& GetMatrix() const { return myTransf.GetModelMatrix(); }

	const UID& GetUID() const { return myUID; }

	void AddComponent(ComponentBase* aComponent);
	ComponentBase* GetComponent(int aType) const;

	void SetVisualObject(VisualObject* aVisualObject) { myVisualObject = aVisualObject; }
	const VisualObject* GetVisualObject() const { return myVisualObject; }
	VisualObject* GetVisualObject() { return myVisualObject; }

	void SetCollisionsEnabled(bool anEnabled) { myCollisionsEnabled = anEnabled; }
	bool GetCollisionsEnabled() const { return myCollisionsEnabled; }

	void CollidedWithTerrain();
	void CollidedWithGO(GameObject* aGo);
	void PreCollision();

	bool IsDead() const { return myIsDead; }
	void Die();

private:
	UID myUID;
	
	Transform myTransf;
	glm::mat4 myCurrentMat;
	glm::vec3 myCenter;

	bool myCollisionsEnabled;
	bool myCollidedWithTerrain;
	bool myIsDead;
	tbb::concurrent_unordered_set<GameObject*> myObjsCollidedWith;

	vector<ComponentBase*> myComponents;
	VisualObject* myVisualObject;
};