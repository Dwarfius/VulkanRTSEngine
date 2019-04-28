#pragma once

class GameObject;

class ComponentBase
{
public:
	ComponentBase() : myOwner(nullptr) {}
	virtual ~ComponentBase() {};

	virtual void Init(GameObject* anOwner) { myOwner = anOwner; };

	GameObject* GetOwner() { return myOwner; }

	// TODO: come up with a better way
	virtual int GetComponentType() { return Type; }
	constexpr static int Type = 0;

protected:
	GameObject* myOwner;
};