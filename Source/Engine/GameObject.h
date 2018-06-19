#pragma once

#include "Graphics.h"
#include "Transform.h"
#include "UID.h"

class Renderer;
class ComponentBase;

// TODO: need to split GameObject into Entity (logic and data) and VisualObject(rendering)
class GameObject
{
	typedef unordered_map<string, Shader::UniformValue> ShaderStorage;

public:
	GameObject(glm::vec3 aPos, glm::vec3 aRot, glm::vec3 aScale);
	~GameObject();

	void Update(float aDeltaTime);

	Transform& GetTransform() { return myTransf; }
	const Transform& GetTransform() const { return myTransf; }
	// TODO: remove duplication - reroute through myTransf
	const glm::mat4& GetMatrix() const { return myCurrentMat; }

	// This is model's center
	glm::vec3 GetCenter() const { return myCenter; }
	float GetRadius() const;

	const UID& GetUID() const { return myUID; }

	const ShaderStorage& GetUniforms() const { return myUniforms; }

	void AddComponent(ComponentBase* aComponent);
	ComponentBase* GetComponent(int aType) const;
	Renderer* GetRenderer() const { return myRenderer; }

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

	// TODO: move this to the Renderer
	ShaderStorage myUniforms;
	size_t myIndex;
	// ==========

	bool myCollisionsEnabled;
	bool myCollidedWithTerrain;
	bool myIsDead;
	tbb::concurrent_unordered_set<GameObject*> myObjsCollidedWith;

	vector<ComponentBase*> myComponents;
	Renderer* myRenderer;
};