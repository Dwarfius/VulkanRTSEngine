#pragma once

#include "Graphics.h"
#include "Transform.h"
#include "UID.h"

class Renderer;
class ComponentBase;

class GameObject
{
public:
	GameObject(vec3 pos, vec3 rot, vec3 scale);
	~GameObject();

	void Update(float deltaTime);

	// TODO: change this to &
	Transform* GetTransform() { return &transf; }
	const Transform* GetTransform() const { return &transf; }
	// This is model's center
	vec3 GetCenter() const { return center; }
	float GetRadius() const;

	const UID& GetUID() const { return myUID; }

	// TODO: fix this up for Vulkan, have it generate/track indices of memory locations
	void SetIndex(size_t newInd) { index = newInd; }
	size_t GetIndex() const { return index; }

	// compiler should perform RVO, so please do it
	// https://web.archive.org/web/20130930101140/http://cpp-next.com/archive/2009/08/want-speed-pass-by-value
	auto GetUniforms() const { return uniforms; }

	void AddComponent(ComponentBase *component);
	ComponentBase* GetComponent(int type) const;
	Renderer* GetRenderer() const { return renderer; }

	void SetCollisionsEnabled(bool val) { collisionsEnabled = val; }
	bool GetCollisionsEnabled() const { return collisionsEnabled; }
	void CollidedWithTerrain();
	void CollidedWithGO(GameObject *go);
	void PreCollision();

	static bool Collide(GameObject *g1, GameObject *g2);

	bool IsDead() const { return dead; }
	void Die();

private:
	UID myUID;

	size_t index = numeric_limits<size_t>::max();
	bool dead = false;
	Transform transf;
	vec3 center;

	unordered_map<string, Shader::UniformValue> uniforms;

	bool collisionsEnabled = true;
	bool collidedWithTerrain = false;
	tbb::concurrent_unordered_set<GameObject*> objsCollidedWith;

	vector<ComponentBase*> components;
	Renderer* renderer = nullptr;
};