#pragma once

#include <Core/Graphics/Texture.h>

class TextureGL : public GPUResource
{
public:
	TextureGL();
	~TextureGL();

	void Bind();

	void Create(any aDescriptor) override;
	bool Upload(any aDescriptor) override;
	void Unload() override;

private:
	uint32_t myGLTexture;
};