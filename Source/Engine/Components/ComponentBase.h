#pragma once

class GameObject;

// TODO: move logic from components to objects which are updated by manager. components will be data/state storages
class ComponentBase
{
public:
	ComponentBase() : myOwner(nullptr) {}

	virtual void Init(GameObject* anOwner) { myOwner = anOwner; };
	virtual void Update(float aDeltaTime) {};
	virtual void Destroy() {};
	virtual void OnCollideWithTerrain() {};
	virtual void OnCollideWithGO(GameObject *other) {};

	GameObject* GetOwner() { return myOwner; }

	virtual int GetComponentType() { return Type; }
	const static int Type;

protected:
	GameObject* myOwner;
};