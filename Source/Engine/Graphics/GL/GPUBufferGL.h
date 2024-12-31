#pragma once

#include <Graphics/Resources/GPUBuffer.h>

class GPUBufferGL final : public GPUBuffer
{
public:
	using GPUBuffer::GPUBuffer;

	void Cleanup() override;

	// Binds the UniformBuffer active range to a binding point. 
	// GLSL uniform block must be bound to the same bind point!
	// Changes OpenGL state, not thread safe.
	void Bind(uint32_t aBindPoint, uint32_t aBindPointType);

private:
	void OnCreate(Graphics& aGraphics) override;
	bool OnUpload(Graphics& aGraphics) override { return true; }
	void OnUnload(Graphics& aGraphics) override;

	uint32_t myBufferGL = 0;
};