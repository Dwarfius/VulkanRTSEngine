#include "Precomp.h"
#include "Model.h"

#include "../GPUResource.h"
#include <Core/Resources/BinarySerializer.h>

#include <TinyObjLoader/tiny_obj_loader.h>
#include <sstream>

Model::Model(PrimitiveType aPrimitiveType, BaseStorage* aStorage, bool aHasIndices)
	: myAABBMin(std::numeric_limits<float>::max())
	, myAABBMax(std::numeric_limits<float>::min())
	, myCenter(0.f)
	, mySphereRadius(0.f)
	, myPrimitiveType(aPrimitiveType)
	, myVertices(aStorage)
	, myHasIndices(aHasIndices)
{
}

Model::Model(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myAABBMin(std::numeric_limits<float>::max())
	, myAABBMax(std::numeric_limits<float>::min())
	, myCenter(0.f)
	, mySphereRadius(0.f)
	, myPrimitiveType(PrimitiveType::Triangles)
	, myVertices(nullptr)
	, myHasIndices(false)
{
}

const Model::BaseStorage* Model::GetBaseVertexStorage() const
{
	ASSERT_STR(myVertices, "Model missing vertex storage!");
	return myVertices;
}

const void* Model::GetVertices() const
{
	ASSERT_STR(myVertices, "Model missing vertex storage!");
	return myVertices->GetRawData();
}

size_t Model::GetVertexCount() const
{
	ASSERT_STR(myVertices, "Model missing vertex storage!");
	return myVertices->GetCount();
}

VertexDescriptor Model::GetVertexDescriptor() const
{
	ASSERT_STR(myVertices, "Model missing vertex storage!");
	return myVertices->GetVertexDescriptor();
}

void Model::SetAABB(glm::vec3 aMin, glm::vec3 aMax)
{
	myAABBMin = aMin;
	myAABBMax = aMax;
	myCenter = (myAABBMin + myAABBMax) / 2.f;
}

void Model::OnLoad(const std::vector<char>& aBuffer, AssetTracker& anAssetTracker)
{
	BinarySerializer serializer(anAssetTracker, true);
	serializer.ReadFrom(aBuffer);

	size_t version;
	serializer.Serialize("myVersion", version);
	ASSERT_STR(version == 1, "Unsupported version!");

	if (Serializer::Scope vertsScope = serializer.SerializeObject("myVertices"))
	{
		VertexDescriptor descriptor;
		if (Serializer::Scope descriptorScope = serializer.SerializeObject("myDescriptor"))
		{
			descriptor.Serialize(serializer);
		}

		if (descriptor == Vertex::GetDescriptor())
		{
			myVertices = new VertStorage<Vertex>(0);
		}
		else
		{
			ASSERT(false);
		}

		myVertices->Serialize(serializer);
	}

	if (Serializer::Scope indexScope = serializer.SerializeArray("myIndices", myIndices))
	{
		for (size_t i = 0; i < myIndices.size(); i++)
		{
			serializer.Serialize(i, myIndices[i]);
		}
	}
	myHasIndices = myIndices.size() > 0;

	serializer.Serialize("myAABBMin", myAABBMin);
	serializer.Serialize("myAABBMax", myAABBMax);
	myCenter = (myAABBMin + myAABBMax) / 2.f;

	serializer.Serialize("mySphereRadius", mySphereRadius);
	serializer.Serialize("myPrimitiveType", myPrimitiveType);
}

void Model::OnSave(std::vector<char>& aBuffer, AssetTracker& anAssetTracker)
{
	ASSERT_STR(myVertices, "Missing vertices!");
	BinarySerializer serializer(anAssetTracker, false);

	size_t version = 1;
	serializer.Serialize("myVersion", version);

	if (Serializer::Scope vertsScope = serializer.SerializeObject("myVertices"))
	{
		// we must serialize the descriptor separately,
		// so that we can determine what vertex storage to create!
		if (Serializer::Scope descriptorScope = serializer.SerializeObject("myDescriptor"))
		{
			GetVertexDescriptor().Serialize(serializer);
		}

		myVertices->Serialize(serializer);
	}

	if (Serializer::Scope indexScope = serializer.SerializeArray("myIndices", myIndices))
	{
		for (size_t i = 0; i < myIndices.size(); i++)
		{
			serializer.Serialize(i, myIndices[i]);
		}
	}

	serializer.Serialize("myAABBMin", myAABBMin);
	serializer.Serialize("myAABBMax", myAABBMax);

	serializer.Serialize("mySphereRadius", mySphereRadius);
	serializer.Serialize("myPrimitiveType", myPrimitiveType);

	serializer.WriteTo(aBuffer);
}

void Serialize(Serializer& aSerializer, Vertex& aVert)
{
	aSerializer.Serialize("myPos", aVert.myPos);
	aSerializer.Serialize("myUv", aVert.myUv);
	aSerializer.Serialize("myNormal", aVert.myNormal);
}