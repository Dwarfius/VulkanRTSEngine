#ifndef _GAME_OBJECT_H
#define _GAME_OBJECT_H

#include "Graphics.h"

class GameObject
{
public:
	GameObject() {}
	~GameObject() {}

	void Update(float deltaTime);

	void SetModel(ModelId newModel) { model = newModel; }
	ModelId GetModel() { return model; }

	void SetShader(ShaderId newShader) { shader = newShader; }
	ShaderId GetShader() { return shader; }

	void SetTexture(TextureId newTexture) { texture = newTexture; }
	TextureId GetTexture() { return texture; }
private:
	ModelId model;
	ShaderId shader;
	TextureId texture;
};

#endif // !_GAME_OBJECT_H