#pragma once

#include <Core/Graphics/Resource.h>

class UniformBufferGL : public GPUResource
{
public:
	struct UploadDescriptor
	{
		size_t mySize;
		const char* myData;
	};

public:
	UniformBufferGL();
	~UniformBufferGL();
	UniformBufferGL(UniformBufferGL&& anOther);

	// Binds the UniformBuffer to a binding point. 
	// GLSL uniform block must be bound to the same bind point!
	// Changes OpenGL state, not thread safe.
	void Bind(uint32_t aBingPoint);

	// Changes OpenGL state, not thread safe.
	void Create(any aDescriptor) override;
	bool Upload(any aDescriptor) override;
	void Unload() override;

private:
	uint32_t myBuffer;
};