#pragma once

class GameObject;

// TODO: move logic from components to objects which are updated by manager. components will be data/state storages
class ComponentBase
{
public:
	ComponentBase() : myOwner(nullptr) {}
	virtual ~ComponentBase() {};

	virtual void Init(GameObject* anOwner) { myOwner = anOwner; };

	GameObject* GetOwner() { return myOwner; }

	virtual int GetComponentType() { return Type; }
	const static int Type;

protected:
	GameObject* myOwner;
};