#include "Precomp.h"
#include "Model.h"

#include "../GPUResource.h"
#include <Core/File.h>

#include <sstream>

size_t Model::GetVertexSize(int aVertexType)
{
	switch (aVertexType)
	{
	case Vertex::Type: return sizeof(Vertex);
	case PosColorVertex::Type: return sizeof(PosColorVertex);
	default: ASSERT(false); return 0;
	}
}

Model::Model(PrimitiveType aPrimitiveType, int aVertexType)
	: myAABBMin(std::numeric_limits<float>::max())
	, myAABBMax(std::numeric_limits<float>::min())
	, myCenter(0.f)
	, mySphereRadius(0.f)
	, myPrimitiveType(aPrimitiveType)
	, myVertType(aVertexType)
	, myVertices(nullptr)
	, myVertexCount(0)
{
}

Model::Model(Resource::Id anId, const std::string& aPath)
	: Resource(anId, kDir.CStr() + aPath)
	, myAABBMin(std::numeric_limits<float>::max())
	, myAABBMax(std::numeric_limits<float>::min())
	, myCenter(0.f)
	, mySphereRadius(0.f)
	, myPrimitiveType(PrimitiveType::Triangles)
	, myVertType(Vertex::Type)
	, myVertices(nullptr)
	, myVertexCount(0)
{
}

void Model::SetAABB(glm::vec3 aMin, glm::vec3 aMax)
{
	myAABBMin = aMin;
	myAABBMax = aMax;
	myCenter = (myAABBMin + myAABBMax) / 2.f;
}

void Model::Update(const UploadDescriptor& aDescChain)
{
	ASSERT_STR(myVertType == aDescChain.myVertexType, "Incompatible descriptor!");
	
	// first need to count how many vertices and indices are there in total
	size_t vertCount = 0;
	size_t indexCount = 0;
	for (const UploadDescriptor* currDesc = &aDescChain;
		currDesc != nullptr;
		currDesc = currDesc->myNextDesc)
	{
		vertCount += currDesc->myVertCount;
		indexCount += currDesc->myIndCount;
	}

	// Maybe the model has been dynamically modified - might need to grow.
	// Definitelly need to allocate if it's our first upload
	if (vertCount > myVertexCount)
	{
		if (myVertices)
		{
			delete[] myVertices;
		}
		myVertexCount = vertCount;
		switch (myVertType)
		{
		case Vertex::Type: myVertices = new Vertex[vertCount]; break;
		case PosColorVertex::Type: myVertices = new PosColorVertex[vertCount]; break;
		default: ASSERT(false);
		}
	}
	myVertexCount = vertCount;
	myIndices.resize(indexCount);

	// Now that we have enough allocated, we can start uploading data from
	// the descriptors
	const size_t vertexSize = GetVertexSize(myVertType);
	size_t vertOffset = 0;
	size_t indOffset = 0;
	char* vertBuffer = static_cast<char*>(myVertices);
	for (const UploadDescriptor* currDesc = &aDescChain;
		currDesc != nullptr;
		currDesc = currDesc->myNextDesc)
	{
		size_t currVertCount = currDesc->myVertCount;
		ASSERT_STR(currVertCount > 0, "Missing additional vertices for a model!");
		std::memcpy(vertBuffer + vertOffset * vertexSize, currDesc->myVertices, currVertCount * vertexSize);
		vertOffset += currVertCount;

		if (indexCount > 0)
		{
			size_t currIndCount = currDesc->myIndCount;
			ASSERT_STR(currIndCount > 0, "Missing additional indices for a model!");
			std::memcpy(myIndices.data() + indOffset, currDesc->myIndices, currIndCount * sizeof(IndexType));
			indOffset += currIndCount;
		}
	}

	myState = State::Ready;
}

void Model::OnLoad(AssetTracker& anAssetTracker, const File& aFile)
{
	ASSERT_STR(myState == State::Uninitialized, "Double load detected!");
	// TODO: extend Model to support more than 1 Vertex type
	ASSERT_STR(myVertType == Vertex::Type, "Unsupported vertex type!");

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
	// this can overallocate if there is vertex duplication
	switch (myVertType)
	{
	case Vertex::Type: myVertices = new Vertex[attrib.vertices.size()]; break;
	case PosColorVertex::Type: myVertices = new PosColorVertex[attrib.vertices.size()]; break;
	default: ASSERT(false);
	}
	
	using VertMap = std::unordered_map<VertexType, IndexType>;
	VertMap uniqueVerts;
	size_t vertsFound = 0;
	indexCount = 0;
	VertexType* vertexBuffer = static_cast<Vertex*>(myVertices);
	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			VertexType vertex;
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
			myIndices[indexCount] = iter->second;
		}
	}

	myCenter = (myAABBMin + myAABBMax) / 2.f;
	mySphereRadius = glm::sqrt(mySphereRadius);
}