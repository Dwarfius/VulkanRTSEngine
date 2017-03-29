#ifndef _GAME_OBJECT_H
#define _GAME_OBJECT_H

#include "Transform.h"
#include "Graphics.h"
#include "Components\ComponentBase.h"
#include "Components\Renderer.h"

class GameObject
{
public:
	~GameObject();

	void Update(float deltaTime);

	Transform* GetTransform() { return &transf; }
	// This is model's center
	vec3 GetCenter() const { return center; }
	float GetRadius() const;

	void SetIndex(size_t newInd) { index = newInd; }
	size_t GetIndex() const { return index; }

	// compiler should perform RVO, so please do it
	// https://web.archive.org/web/20130930101140/http://cpp-next.com/archive/2009/08/want-speed-pass-by-value
	auto GetUniforms() const { return uniforms; }

	void AddComponent(ComponentBase *component);
	Renderer* GetRenderer() const { return renderer; }

	void SetCollisionsEnabled(bool val) { collisionsEnabled = val; }
	bool GetCollisionsEnabled() const { return collisionsEnabled; }
	void CollidedWithTerrain();
	void CollidedWithGO(GameObject *go);

	static bool Collide(GameObject *g1, GameObject *g2);

	bool IsDead() const { return dead; }
	void Die() { dead = true; }

private:
	size_t index;
	bool dead = false;
	Transform transf;
	vec3 center;

	bool collisionsEnabled = true;

	unordered_map<string, Shader::UniformValue> uniforms;

	vector<ComponentBase*> components;
	Renderer *renderer;
};

#endif // !_GAME_OBJECT_H