#pragma once

#include "Resource.h"
#include "Vertex.h"

// A class representing a generic mesh, plus a couple utility methods to get info about the model.
// Directly contstruct this class if you just need it on the CPU, otherwise use
// Graphics::AllocateMesh() to get an instance that will be uploaded to the GPU.
// Model retains all memory for the duration of it's lifetime.
class Model : public Resource
{
public:
	using IndexType = uint32_t;

	struct CreateDescriptor
	{
		GPUResource::Primitive myPrimitiveType;
		GPUResource::Usage myUsage;
		int myVertType;
		bool myIsIndexed;
	};

	struct UploadDescriptor
	{
		size_t myPrimitiveCount;
		const void* myVertices;
		size_t myVertCount;
		IndexType* myIndices; // optional
		size_t myIndCount; // optional
	};

public:
	Model(Resource::Id anId);
	Model(Resource::Id anId, const string& aPath);

	// Method for manually providing the model with data.
	// anAABBMin and anAABBMax form axis-aligned bounding box,
	// while aSphereRadius is the bounding sphere radius
	// After calling it, the model will switch it state to PendingUpload
	void SetData(vector<Vertex>&& aVerts, vector<IndexType>&& aIndices,
		glm::vec3 anAABBMin, glm::vec3 anAABBMax, float aSphereRadius);

	Resource::Type GetResType() const override { return Resource::Type::Model; }

	const Vertex* GetVertices() const { return myVertices.data(); }
	const IndexType* GetIndices() const { return myIndices.data(); }

	size_t GetVertexCount() const { return myVertices.size(); }
	size_t GetIndexCount() const { return myIndices.size(); }

	// Returns model center point
	glm::vec3 GetCenter() const { return myCenter; }
	// Returns bounding sphere radius in model space
	float GetSphereRadius() const { return mySphereRadius; }
	
	// Returns the min corner of AABB in model space
	glm::vec3 GetAABBMin() const { return myAABBMin; }
	// Returns the max corner of AABB in model space
	glm::vec3 GetAABBMax() const { return myAABBMax; }

private:
	// Processes an .obj file in folder "assets/objects/".
	void OnLoad(AssetTracker& anAssetTracker) override;
	// Uploads to GPU
	void OnUpload(GPUResource* aGPURes) override;

	// TODO: this is potentially wasteful - not all models can have UVs or normals
	vector<Vertex> myVertices;
	vector<IndexType> myIndices;

	glm::vec3 myAABBMin;
	glm::vec3 myAABBMax;
	glm::vec3 myCenter;
	float mySphereRadius;
};