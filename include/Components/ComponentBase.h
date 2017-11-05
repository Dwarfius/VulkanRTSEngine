#pragma once

class GameObject;

class ComponentBase
{
public:
	virtual void Init(GameObject *o) { owner = o; };
	virtual void Update(float deltaTime) {};
	virtual void Destroy() {};
	virtual void OnCollideWithTerrain() {};
	virtual void OnCollideWithGO(GameObject *other) {};

	GameObject* GetOwner() { return owner; }

	virtual int GetComponentType() { return Type; }
	const static int Type;

protected:
	GameObject *owner;
};