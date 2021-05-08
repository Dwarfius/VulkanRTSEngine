#include "Precomp.h"
#include "Model.h"

#include "../GPUResource.h"
#include <Core/Resources/BinarySerializer.h>

#include <TinyObjLoader/tiny_obj_loader.h>
#include <sstream>

Model::Model(PrimitiveType aPrimitiveType, BaseStorage* aStorage, bool aHasIndices)
	: myPrimitiveType(aPrimitiveType)
	, myVertices(aStorage)
	, myHasIndices(aHasIndices)
{
}

Model::Model(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
{
}

Model::~Model()
{
	if (myVertices)
	{
		delete myVertices;
	}
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

void Model::Serialize(Serializer& aSerializer)
{
	size_t version = 1;
	aSerializer.Serialize("myVersion", version);
	ASSERT_STR(version == 1, "Unsupported version!");

	if (Serializer::Scope vertsScope = aSerializer.SerializeObject("myVertices"))
	{
		VertexDescriptor descriptor;
		if (myVertices)
		{
			descriptor = GetVertexDescriptor();
		}
		if (Serializer::Scope descriptorScope = aSerializer.SerializeObject("myDescriptor"))
		{
			descriptor.Serialize(aSerializer);
		}

		if (aSerializer.IsReading())
		{
			if (myVertices)
			{
				delete myVertices;
			}

			if (descriptor == Vertex::GetDescriptor())
			{
				myVertices = new VertStorage<Vertex>(0);
			}
			else
			{
				ASSERT(false);
			}
		}

		myVertices->Serialize(aSerializer);
	}

	if (Serializer::Scope indexScope = aSerializer.SerializeArray("myIndices", myIndices))
	{
		for (size_t i = 0; i < myIndices.size(); i++)
		{
			aSerializer.Serialize(i, myIndices[i]);
		}
	}
	myHasIndices = myIndices.size() > 0;

	aSerializer.Serialize("myAABBMin", myAABBMin);
	aSerializer.Serialize("myAABBMax", myAABBMax);
	myCenter = (myAABBMin + myAABBMax) / 2.f;

	aSerializer.Serialize("mySphereRadius", mySphereRadius);
	aSerializer.Serialize("myPrimitiveType", myPrimitiveType);
}

void Serialize(Serializer& aSerializer, Vertex& aVert)
{
	aSerializer.Serialize("myPos", aVert.myPos);
	aSerializer.Serialize("myUv", aVert.myUv);
	aSerializer.Serialize("myNormal", aVert.myNormal);
}