#pragma once

#include "ComponentBase.h"

class Renderer : public ComponentBase
{
public:
	Renderer(ModelId aModelId, ShaderId aShaderId, TextureId aTextureId) 
		: myModel(aModelId), myShader(aShaderId), myTexture(aTextureId) {}

	void SetModel(ModelId aNewModel) { myModel = aNewModel; }
	ModelId GetModel() const { return myModel; }

	void SetShader(ShaderId aNewShader) { myShader = aNewShader; }
	const ShaderId GetShader() const { return myShader; }

	void SetTexture(TextureId aNewTexture) { myTexture = aNewTexture; }
	TextureId GetTexture() const { return myTexture; }

private:
	ModelId myModel;
	ShaderId myShader;
	TextureId myTexture;
};