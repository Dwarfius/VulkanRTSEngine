#ifndef _RENDERER_H
#define _RENDERER_H

#include "Components/ComponentBase.h"
#include "Graphics.h"

class Renderer : public ComponentBase
{
public:
	Renderer(ModelId m, ShaderId s, TextureId t) : model(m), shader(s), texture(t) {}

	void SetModel(ModelId newModel) { model = newModel; }
	ModelId GetModel() const { return model; }

	void SetShader(ShaderId newShader) { shader = newShader; }
	const ShaderId GetShader() const { return shader; }

	void SetTexture(TextureId newTexture) { texture = newTexture; }
	TextureId GetTexture() const { return texture; }

private:
	ModelId model;
	ShaderId shader;
	TextureId texture;
};

#endif