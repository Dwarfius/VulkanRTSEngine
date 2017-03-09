#ifndef _COMPONENT_BASE_H
#define _COMPONENT_BASE_H

class GameObject;

class ComponentBase
{
public:
	virtual void Init(GameObject *o);
	virtual void Update(float deltaTime);
	virtual void Destroy();

	GameObject* GetOwner() { return owner; }
protected:
	GameObject *owner;
};

#endif