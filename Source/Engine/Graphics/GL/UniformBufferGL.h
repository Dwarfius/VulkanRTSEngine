#pragma once

#include "Graphics/Resource.h"

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

	void Bind(uint32_t aBingPoint);

	// Changes OpenGL state, not thread safe.
	void Create(any aDescriptor) override;
	bool Upload(any aDescriptor) override;
	void Unload() override;

private:
	uint32_t myBuffer;
};