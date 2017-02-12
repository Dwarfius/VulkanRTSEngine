#ifndef _GAME_OBJECT_H
#define _GAME_OBJECT_H

#include "Graphics.h"

class GameObject
{
public:
	GameObject() : pos(vec3(0, 0, 0)), size(vec3(1, 1, 1)) {}
	~GameObject() {}

	void Update(float deltaTime);

	void SetModel(ModelId newModel) { model = newModel; }
	ModelId GetModel() { return model; }

	void SetShader(ShaderId newShader) { shader = newShader; }
	ShaderId GetShader() { return shader; }

	void SetTexture(TextureId newTexture) { texture = newTexture; }
	TextureId GetTexture() { return texture; }

	vec3 GetPos() { return pos; }
	void SetPos(vec3 pPos) { pos = pPos; modelDirty = true; }
	void Move(vec3 delta) { pos += delta; modelDirty = true; }

	vec3 GetRotation() { return rotation; }
	void SetRotation(vec3 pRotation) { rotation = pRotation; modelDirty = true; }
	void AddRotation(vec3 delta) { rotation += delta; modelDirty = true; }

	vec3 GetScale() { return size; }
	void SetScale(vec3 pScale) { size = pScale; modelDirty = true; }
	void AddScale(vec3 delta) { size += delta; modelDirty = true; }

	mat4 GetModelMatrix() { if (modelDirty) UpdateMatrix(); return modelMatrix; }

private:
	ModelId model;
	ShaderId shader;
	TextureId texture;

	vec3 pos, rotation, size;
	mat4 modelMatrix;
	bool modelDirty = true;
	void UpdateMatrix();
};

#endif // !_GAME_OBJECT_H