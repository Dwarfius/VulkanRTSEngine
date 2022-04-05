#pragma once

#include <Graphics/Resources/UniformBuffer.h>

class UniformBufferGL final : public UniformBuffer
{
public:
	using UniformBuffer::UniformBuffer;

	// Binds the UniformBuffer active range to a binding point. 
	// GLSL uniform block must be bound to the same bind point!
	// Changes OpenGL state, not thread safe.
	void Bind(uint32_t aBingPoint);

	// Doesn't change OpenGL state
	char* Map() override;
	void Unmap() override;

private:
	void OnCreate(Graphics& aGraphics) override;
	bool OnUpload(Graphics& aGraphics) override { return true; }
	void OnUnload(Graphics& aGraphics) override;

	uint32_t myUniformBuffer = 0;
};