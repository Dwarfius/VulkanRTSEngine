#pragma once

#include <Core/Resources/Resource.h>
#include <Core/Resources/Serializer.h>
#include "../Interfaces/IModel.h"

class File;

class Model : public Resource, public IModel
{
public:
	constexpr static StaticString kExtension = ".model";

	template<class T>
	struct UploadDescriptor
	{
		UploadDescriptor()
			: myVertsOwned(false)
			, myIndOwned(false)
		{}

		const T* myVertices = nullptr;
		size_t myVertCount = 0;
		const IndexType* myIndices = nullptr; // optional
		size_t myIndCount = 0; // optional
		UploadDescriptor<T>* myNextDesc = nullptr; // optional, used to chain multiple uploads together
		bool myVertsOwned : 4; // indicates whether myVertices in this descriptor need cleaning up
		bool myIndOwned : 4; // indicates whether myIndices in this descriptor need cleaning up
	};

	class BaseStorage
	{
	public:
		const void* GetRawData() const { return myData; }

		void SetCount(size_t aCount) { myCount = aCount; }
		size_t GetCount() const { return myCount; }

		size_t GetCapacity() const { return myCapacity; }
		VertexDescriptor GetVertexDescriptor() const { return myVertDesc; }
		virtual void Serialize(Serializer& aSerializer) = 0;
		virtual ~BaseStorage() = default;

	protected:
		void* myData = nullptr;
		size_t myCount = 0;
		size_t myCapacity = 0;
		VertexDescriptor myVertDesc;
	};
	template<class T>
	class VertStorage : public BaseStorage
	{
	public:
		VertStorage(size_t aCount)
		{
			myVertDesc = T::GetDescriptor();
			myCount = aCount;
			myCapacity = myCount;
			if (myCount)
			{
				myData = new T[myCapacity];
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

		void Serialize(Serializer& aSerializer) final;
	private:
		friend class Model;
		T* GetData() { return static_cast<T*>(myData); }
	};

public:
	template<class VertType, size_t VertSpanSize>
	Model(PrimitiveType aPrimitiveType, std::span<VertType, VertSpanSize> aVerts, bool aHasIndices);

	template<class VertType, size_t VertSpanSize, size_t IndexSpanSize>
	Model(PrimitiveType aPrimitiveType, std::span<VertType, VertSpanSize> aVerts, std::span<IndexType, IndexSpanSize> aIndices);

	Model(Resource::Id anId, std::string_view aPath);
	~Model();

	// Full access to typed vertex storage of a model
	template<class T>
	const VertStorage<T>* GetVertexStorage() const;
	// Full access to base vertex storage of a model
	const BaseStorage* GetBaseVertexStorage() const;

	// vertex-data shortcut accessors to generic vertex storage
	const void* GetVertices() const;
	// vertex-count shortcut accessors to generic vertex storage
	size_t GetVertexCount() const;
	// vertex-descriptor shortcut accessors to generic vertex storage
	VertexDescriptor GetVertexDescriptor() const;

	const IndexType* GetIndices() const { return myIndices.data(); }
	size_t GetIndexCount() const { return myIndices.size(); }
	bool HasIndices() const { return myHasIndices; }

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

	std::string_view GetTypeName() const override { return "Model"; }

private:
	bool PrefersBinarySerialization() const final { return true; }
	void Serialize(Serializer& aSerializer) final;

	BaseStorage* myVertices = nullptr;
	std::vector<IndexType> myIndices;
	glm::vec3 myAABBMin = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 myAABBMax = glm::vec3(std::numeric_limits<float>::min());
	glm::vec3 myCenter = glm::vec3(0.f);
	float mySphereRadius = 0.f;
	PrimitiveType myPrimitiveType = PrimitiveType::Triangles;
	bool myHasIndices = false;
};

template<class VertType, size_t VertSpanSize>
Model::Model(PrimitiveType aPrimitiveType, std::span<VertType, VertSpanSize> aVerts, bool aHasIndices)
	: myPrimitiveType(aPrimitiveType)
	, myVertices(new VertStorage<VertType>(aVerts.size()))
	, myHasIndices(aHasIndices)
{
	VertType* vertices = static_cast<VertStorage<VertType>*>(myVertices)->GetData();
	std::memcpy(vertices, aVerts.data(), aVerts.size_bytes());
}

template<class VertType, size_t VertSpanSize, size_t IndexSpanSize>
Model::Model(PrimitiveType aPrimitiveType, std::span<VertType, VertSpanSize> aVerts, std::span<IndexType, IndexSpanSize> anIndices)
	: Model(aPrimitiveType, aVerts, true)
{
	myIndices.resize(anIndices.size());
	IndexType* indices = myIndices.data();
	std::memcpy(indices, anIndices.data(), anIndices.size_bytes());
}

template<class T>
const Model::VertStorage<T>* Model::GetVertexStorage() const
{
	ASSERT_STR(myVertices, "Uninitialized Model!");
	ASSERT_STR(T::GetDescriptor() == myVertices->GetVertexDescriptor(), "Wrong Vertex Type!");
	return static_cast<const VertStorage<T>*>(myVertices);
}

template<class T>
void Model::Update(const UploadDescriptor<T> & aDescChain)
{
	ASSERT_STR(!myVertices || myVertices->GetVertexDescriptor() == T::GetDescriptor(), "Incompatible descriptor!");

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
	myVertices->SetCount(vertCount);
	myIndices.resize(indexCount);
	myHasIndices = indexCount > 0;

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

		if (currDesc->myVertsOwned)
		{
			delete[] currDesc->myVertices;
		}

		if (indexCount > 0)
		{
			size_t currIndCount = currDesc->myIndCount;
			ASSERT_STR(currIndCount > 0, "Missing additional indices for a model!");
			std::memcpy(myIndices.data() + indOffset, currDesc->myIndices, currIndCount * sizeof(IndexType));
			indOffset += currIndCount;

			if (currDesc->myIndOwned)
			{
				delete[] currDesc->myIndices;
			}
		}
	}
}

namespace SerializeHelpers
{
	template<class T> void Serialize(Serializer&, T&)
	{
		ASSERT_STR(false, "Tried to serialize an unsupported type! Are you missing a specialization?");
	}
}

void Serialize(Serializer& aSerializer, Vertex& aVert);

template<class T>
void Model::VertStorage<T>::Serialize(Serializer& aSerializer)
{
	size_t oldCount = myCount;
	if (Serializer::ArrayScope vertsScope{ aSerializer, "myVerts", myCount })
	{
		if (aSerializer.IsReading() && oldCount != myCount)
		{
			if (myData)
			{
				delete[] myData;
			}
			myData = new T[myCount];
		}
		T* verts = GetData();
		for (size_t i = 0; i < myCount; i++)
		{
			if (Serializer::ObjectScope vertScope{ aSerializer, Serializer::kArrayElem })
			{
				using SerializeHelpers::Serialize;
				Serialize(aSerializer, verts[i]);
			}
		}
	}
}