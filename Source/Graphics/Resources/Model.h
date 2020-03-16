#pragma once

#include "../Resource.h"
#include "../Interfaces/IModel.h"

class File;

class Model : public Resource, public IModel
{
public:
	static constexpr StaticString kDir = Resource::AssetsFolder + "objects/";

	template<class T>
	struct UploadDescriptor
	{
		size_t myPrimitiveCount;
		const T* myVertices;
		size_t myVertCount;
		const IndexType* myIndices; // optional
		size_t myIndCount; // optional
		UploadDescriptor<T>* myNextDesc; // optional, used to chain multiple uploads together
	};

	class BaseStorage
	{
	public:
		const void* GetRawData() const { return myData; }
		size_t GetCount() const { return myCount; }
		int GetType() const { return myType; }

	protected:
		void* myData;
		size_t myCount;
		int myType;
	};
	template<class T>
	class VertStorage : public BaseStorage
	{
	public:
		VertStorage(size_t aCount)
		{
			myType = T::Type;
			myCount = aCount;
			if (myCount)
			{
				myData = new T[myCount];
			}
		}

		~VertStorage()
		{
			if (myData)
			{
				delete myData;
			}
		}

		const T* GetData() const { return static_cast<const T*>(myData); }
	private:
		friend class Model;
		T* GetData() { return static_cast<T*>(myData); }
	};

	static size_t GetVertexSize(int aVertexType);

public:
	Model(PrimitiveType aPrimitiveType, int aVertexType);
	Model(Resource::Id anId, const std::string& aPath);

	Resource::Type GetResType() const override { return Resource::Type::Model; }

	// Full access to typed vertex storage of a model
	template<class T>
	const VertStorage<T>* GetVertexStorage() const;
	// Full access to base vertex storage of a model
	const BaseStorage* GetBaseVertexStorage() const;

	// vertex-data shortcut accessors to generic vertex storage
	const void* GetVertices() const;
	// vertex-count shortcut accessors to generic vertex storage
	size_t GetVertexCount() const;
	// vertex-type shortcut accessors to generic vertex storage
	int GetVertexType() const;

	const IndexType* GetIndices() const { return myIndices.data(); }
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

	template<class T>
	void Update(const UploadDescriptor<T>& aDescChain);

private:
	// Processes an .obj file in folder "../assets/objects/".
	void OnLoad(AssetTracker& anAssetTracker, const File& aFile) override;

	BaseStorage* myVertices;
	std::vector<IndexType> myIndices;
	glm::vec3 myAABBMin;
	glm::vec3 myAABBMax;
	glm::vec3 myCenter;
	float mySphereRadius;
	PrimitiveType myPrimitiveType;
};

template<class T>
const Model::VertStorage<T>* Model::GetVertexStorage() const
{
	ASSERT_STR(myVertices, "Uninitialized Model!");
	ASSERT_STR(T::Type == myVertices->GetType(), "Wrong Vertex Type!");
	return static_cast<const VertStorage<T>*>(myVertices);
}

template<class T>
void Model::Update(const UploadDescriptor<T> & aDescChain)
{
	ASSERT_STR(!myVertices || myVertices->GetType() == T::Type, "Incompatible descriptor!");

	// first need to count how many vertices and indices are there in total
	size_t vertCount = 0;
	size_t indexCount = 0;
	for (const UploadDescriptor<T>* currDesc = &aDescChain;
		currDesc != nullptr;
		currDesc = currDesc->myNextDesc)
	{
		vertCount += currDesc->myVertCount;
		indexCount += currDesc->myIndCount;
	}

	// Maybe the model has been dynamically modified - might need to grow.
	// Definitelly need to allocate if it's our first upload
	if (!myVertices || vertCount > myVertices->GetCount())
	{
		if (myVertices)
		{
			delete myVertices;
		}
		myVertices = new VertStorage<T>(vertCount);
	}
	myIndices.resize(indexCount);

	// Now that we have enough allocated, we can start uploading data from
	// the descriptors
	size_t vertOffset = 0;
	size_t indOffset = 0;
	T* vertBuffer = static_cast<VertStorage<T>*>(myVertices)->GetData();
	for (const UploadDescriptor<T>* currDesc = &aDescChain;
		currDesc != nullptr;
		currDesc = currDesc->myNextDesc)
	{
		if (currDesc->myVertCount == 0)
		{
			continue;
		}

		size_t currVertCount = currDesc->myVertCount;
		std::memcpy(vertBuffer + vertOffset, currDesc->myVertices, currVertCount * sizeof(T));
		vertOffset += currVertCount;

		if (indexCount > 0)
		{
			size_t currIndCount = currDesc->myIndCount;
			ASSERT_STR(currIndCount > 0, "Missing additional indices for a model!");
			std::memcpy(myIndices.data() + indOffset, currDesc->myIndices, currIndCount * sizeof(IndexType));
			indOffset += currIndCount;
		}
	}
}