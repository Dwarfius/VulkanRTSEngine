#pragma once

#include "../Resource.h"
#include "../Interfaces/IModel.h"

class File;

class Model : public Resource, public IModel
{
public:
	static constexpr StaticString kDir = Resource::AssetsFolder + "objects/";

	struct UploadDescriptor
	{
		size_t myPrimitiveCount;
		int myVertexType;
		const void* myVertices; // TODO: fix this!
		size_t myVertCount;
		IndexType* myIndices; // optional
		size_t myIndCount; // optional
		UploadDescriptor* myNextDesc; // optional, used to chain multiple uploads together
	};

	static size_t GetVertexSize(int aVertexType);

public:
	Model(PrimitiveType aPrimitiveType, int aVertexType);
	Model(Resource::Id anId, const std::string& aPath);

	Resource::Type GetResType() const override { return Resource::Type::Model; }

	int GetVertexType() const { return myVertType; }
	const void* GetVertices() const { return myVertices; }
	const IndexType* GetIndices() const { return myIndices.data(); }

	size_t GetVertexCount() const { return myVertexCount; }
	size_t GetIndexCount() const { return myIndices.size(); }

	// Returns model center point
	glm::vec3 GetCenter() const override final { return myCenter; }
	// Returns bounding sphere radius in model space
	float GetSphereRadius() const override final { return mySphereRadius; }
	void SetSphereRadius(float aRadius) { mySphereRadius = aRadius; }

	// Returns the min corner of AABB in model space
	glm::vec3 GetAABBMin() const { return myAABBMin; }
	// Returns the max corner of AABB in model space
	glm::vec3 GetAABBMax() const { return myAABBMax; }
	void SetAABB(glm::vec3 aMin, glm::vec3 aMax);

	PrimitiveType GetPrimitiveType() const { return myPrimitiveType; }

	void Update(const UploadDescriptor& aDescChain);

private:
	// Processes an .obj file in folder "../assets/objects/".
	void OnLoad(AssetTracker& anAssetTracker, const File& aFile) override;

	void* myVertices; // TODO: Get rid of this via templates!
	size_t myVertexCount;
	std::vector<IndexType> myIndices;
	glm::vec3 myAABBMin;
	glm::vec3 myAABBMax;
	glm::vec3 myCenter;
	float mySphereRadius;
	PrimitiveType myPrimitiveType;
	int myVertType;
};