#pragma once

class GameObject;

class ComponentBase
{
public:
	ComponentBase() : myOwner(nullptr) {}
	virtual ~ComponentBase() {};

	virtual void Init(GameObject* anOwner) { myOwner = anOwner; };

	GameObject* GetOwner() { return myOwner; }

protected:
	GameObject* myOwner;
};