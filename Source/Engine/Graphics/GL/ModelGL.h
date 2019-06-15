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
	// Allocates a new buffer for vertices and uploads the data
	void AllocateVertices(size_t aTotalCount);
	// Allocates a new buffer for indices and uploads the data
	void AllocateIndices(size_t aTotalCount);

	// Uploads the vertex data directly to the buffer(avoid allocations)
	void UploadVertices(const void* aVertices, size_t aVertexCount, size_t anOffset);
	// Uploads the index data directly to the buffer(avoids allocations)
	void UploadIndices(const Model::IndexType* aIndices, size_t aIndexCount, size_t anOffset);

	uint32_t myVAO; // vertex array
	uint32_t myVBO; // vertex buffer
	uint32_t myEBO; // index buffer, optional

	uint32_t myDrawMode;
	size_t myPrimitiveCount;
	size_t myVertCount;
	size_t myIndexCount;
	GPUResource::Usage myUsage;
	int myVertType;
};