#include "Precomp.h"
#include "Model.h"

#include "../GPUResource.h"
#include <Core/File.h>

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

int Model::GetVertexType() const
{
	ASSERT_STR(myVertices, "Model missing vertex storage!");
	return myVertices->GetType();
}

void Model::SetAABB(glm::vec3 aMin, glm::vec3 aMax)
{
	myAABBMin = aMin;
	myAABBMax = aMax;
	myCenter = (myAABBMin + myAABBMax) / 2.f;
}

void Model::OnLoad(const File& aFile)
{
	// TODO: extend Model to support more than 1 Vertex type loading
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	// TODO: write own obj loader - this causes a full copy, which is wasteful,
	// and with STL's new codecvt ryu float parser implementation I think I can get
	// faster results
	std::istringstream buf(aFile.GetBuffer());
	tinyobj::MaterialFileReader matFileReader(kDir.CStr());
	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &buf, &matFileReader);
	if (!loaded)
	{
		SetErrMsg(move(err));
		return;
	}

	if (!err.empty())
	{
		printf("[Warning] Model loading warning: %s\n", err.c_str());
	}

	// quickly count indices
	size_t indexCount = 0;
	for (const tinyobj::shape_t& shape : shapes)
	{
		indexCount += shape.mesh.indices.size();
	}
	myIndices.resize(indexCount);
	myHasIndices = indexCount > 0;

	VertStorage<Vertex>* vertStorage = new VertStorage<Vertex>(attrib.vertices.size());
	myVertices = vertStorage;
	
	auto vertEq = [](const Vertex& aLeft, const Vertex& aRight) {
		return aLeft.myPos == aRight.myPos
			&& aLeft.myUv == aRight.myUv
			&& aLeft.myNormal == aRight.myNormal;
	};
	using VertMap = std::unordered_map<Vertex, IndexType, std::hash<Vertex>, decltype(vertEq)>;
	VertMap uniqueVerts(0, std::hash<Vertex>(), vertEq);
	size_t vertsFound = 0;
	indexCount = 0;
	Vertex* vertexBuffer = vertStorage->GetData();
	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			Vertex vertex;
			vertex.myPos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if (index.texcoord_index != -1)
			{
				vertex.myUv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1 - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (index.normal_index != -1)
			{
				vertex.myNormal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			// checking for vertex duplication
			VertMap::iterator iter = uniqueVerts.find(vertex);
			if (iter == uniqueVerts.end())
			{
				// update the bounds
				myAABBMin.x = glm::min(myAABBMin.x, vertex.myPos.x);
				myAABBMin.y = glm::min(myAABBMin.y, vertex.myPos.y);
				myAABBMin.z = glm::min(myAABBMin.z, vertex.myPos.z);

				myAABBMax.x = glm::max(myAABBMax.x, vertex.myPos.x);
				myAABBMax.y = glm::max(myAABBMax.y, vertex.myPos.y);
				myAABBMax.z = glm::max(myAABBMax.z, vertex.myPos.z);

				// update radius by finding the furthest away vert
				mySphereRadius = glm::max(mySphereRadius, glm::length2(vertex.myPos));

				// push back the new vertex and record it's position
				ASSERT_STR(vertsFound < std::numeric_limits<IndexType>::max(), "Vertex index overflow!");
				// TODO: this insert with a hint is implementation dependent (might be optimization,
				// might be a slowdown cause bad hint). Need to come back later and reread implementation
				// marking that new vertex is at this index
				iter = uniqueVerts.insert(uniqueVerts.begin(), { vertex, static_cast<IndexType>(vertsFound) });
				vertexBuffer[vertsFound] = vertex; // adding it at the marked position
				vertsFound++;
			}

			// reusing the vertex
			myIndices[indexCount++] = iter->second;
		}
	}

	myCenter = (myAABBMin + myAABBMax) / 2.f;
	mySphereRadius = glm::sqrt(mySphereRadius);
}