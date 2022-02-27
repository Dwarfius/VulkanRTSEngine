#pragma once

#include <Graphics/GPUResource.h>
#include <Graphics/Descriptor.h>

class UniformBufferGL : public GPUResource
{
public:
	struct UploadDescriptor
	{
		size_t mySize;
		const char* myData;
	};

public:
	UniformBufferGL(const Descriptor& aDescriptor);

	// Binds the UniformBuffer to a binding point. 
	// GLSL uniform block must be bound to the same bind point!
	// Changes OpenGL state, not thread safe.
	void Bind(uint32_t aBingPoint);

private:
	void OnCreate(Graphics& aGraphics) override final;
	bool OnUpload(Graphics& aGraphics) override final;
	void OnUnload(Graphics& aGraphics) override final;

	Descriptor myDescriptor;
	uint32_t myBuffer;
	UploadDescriptor myUploadDesc;

	friend class RenderPassJobGL;
	void UploadData(const UploadDescriptor& anUploadDesc);
};