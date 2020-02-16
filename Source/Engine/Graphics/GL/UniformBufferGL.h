#pragma once

#include <Graphics/GPUResource.h>

class UniformBufferGL : public GPUResource
{
public:
	struct UploadDescriptor
	{
		size_t mySize;
		const char* myData;
	};

public:
	UniformBufferGL(size_t aBufferSize);

	// Binds the UniformBuffer to a binding point. 
	// GLSL uniform block must be bound to the same bind point!
	// Changes OpenGL state, not thread safe.
	void Bind(uint32_t aBingPoint);

private:
	void OnCreate(Graphics& aGraphics) override final;
	bool OnUpload(Graphics& aGraphics) override final;
	void OnUnload(Graphics& aGraphics) override final;

	size_t myBufferSize;
	uint32_t myBuffer;
	UploadDescriptor myUploadDesc;

	friend class RenderPassJobGL;
	void UploadData(const UploadDescriptor& anUploadDesc);
};