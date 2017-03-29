#ifndef _COMPONENT_BASE_H
#define _COMPONENT_BASE_H

class GameObject;

class ComponentBase
{
public:
	virtual void Init(GameObject *o);
	virtual void Update(float deltaTime);
	virtual void Destroy();
	virtual void OnCollideWithTerrain();
	virtual void OnCollideWithGO(GameObject *other);

	GameObject* GetOwner() { return owner; }
protected:
	GameObject *owner;
};

#endif