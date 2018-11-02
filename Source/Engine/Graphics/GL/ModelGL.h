#pragma once

#include <Graphics/Model.h>

// TODO: create templated version of functions for different Vertex types
class ModelGL : public GPUResource
{
public:
	ModelGL();
	// Deallocates OpenGL resources. Not thread safe.
	~ModelGL();

	// Bind the current GL resources for model drawing
	void Bind();

	uint32_t GetDrawMode() const { return myDrawMode; }
	size_t GetPrimitiveCount() const { return myPrimitiveCount; }

	// Creates OpenGL resources. Changes OpenGL state - not thread-safe.
	void Create(any aDescriptor) override;
	bool Upload(any aDescriptor) override;
	void Unload() override;

private:
	uint32_t myVAO; // vertex array
	uint32_t myVBO; // vertex buffer
	uint32_t myEBO; // index buffer, optional

	uint32_t myDrawMode;
	size_t myPrimitiveCount;
	GPUResource::Usage myUsage;
	int myVertType;
};