#pragma once

#include <Graphics/Resources/UniformBuffer.h>

class UniformBufferGL final : public UniformBuffer
{
public:
	using UniformBuffer::UniformBuffer;

	void Cleanup() override;

	// Binds the UniformBuffer active range to a binding point. 
	// GLSL uniform block must be bound to the same bind point!
	// Changes OpenGL state, not thread safe.
	void Bind(uint32_t aBingPoint);

private:
	void OnCreate(Graphics& aGraphics) override;
	bool OnUpload(Graphics& aGraphics) override { return true; }
	void OnUnload(Graphics& aGraphics) override;

	uint32_t myUniformBuffer = 0;
};