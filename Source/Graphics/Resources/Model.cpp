#include "Precomp.h"
#include "Model.h"

#include "../GPUResource.h"
#include <Core/File.h>

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

void Model::OnLoad(const File& aFile)
{
	ASSERT_STR(false, "Direct model serialization is NYI, use OBJResource instead!");
}