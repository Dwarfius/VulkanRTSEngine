#pragma once

#include <Graphics/Resources/GPUModel.h>

// TODO: create templated version of functions for different Vertex types
class ModelGL : public GPUModel
{
public:
	ModelGL(UsageType aUsage);
	ModelGL(PrimitiveType aPrimType, UsageType aUsage, const VertexDescriptor& aVertDesc, bool aIsIndexed);

	// Bind the current GL resources for model drawing
	void Bind();

	uint32_t GetDrawMode() const { return myDrawMode; }
	size_t GetVertexCount() const { return myVertCount; }

private:
	void OnCreate(Graphics& aGraphics) override final;
	bool OnUpload(Graphics& aGraphics) override final;
	void OnUnload(Graphics& aGraphics) override final;

	// Allocates a new buffer for vertices and uploads the data
	void AllocateVertices(size_t aTotalCount);
	// Allocates a new buffer for indices and uploads the data
	void AllocateIndices(size_t aTotalCount);

	// Uploads the vertex data directly to the buffer(avoids allocations)
	void UploadVertices(const void* aVertices, size_t aVertexCount, size_t anOffset);
	// Uploads the index data directly to the buffer(avoids allocations)
	void UploadIndices(const IndexType* aIndices, size_t aIndexCount, size_t anOffset);

	VertexDescriptor myVertDescriptor;

	uint32_t myVAO; // vertex array
	uint32_t myVBO; // vertex buffer
	uint32_t myEBO; // index buffer, optional

	uint32_t myDrawMode;
	size_t myVertCount;
	size_t myIndexCount;
	UsageType myUsage;
	bool myIsIndexed;
};