#include "Precomp.h"
#include "Model.h"

Model::Model(Resource::Id anId)
	: Model(anId, "")
{
}

Model::Model(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myAABBMin(std::numeric_limits<float>::max())
	, myAABBMax(std::numeric_limits<float>::min())
	, myCenter(0.f)
	, mySphereRadius(0.f)
	, myVertices()
	, myIndices()
{
}

void Model::SetData(std::vector<Vertex>&& aVerts, std::vector<IndexType>&& aIndices,
	glm::vec3 anAABBMin, glm::vec3 anAABBMax, float aSphereRadius)
{
	myVertices = move(aVerts);
	myIndices = move(aIndices);
	myAABBMin = anAABBMin;
	myAABBMax = anAABBMax;
	myCenter = (myAABBMin + myAABBMax) / 2.f;
	mySphereRadius = aSphereRadius;

	myState = State::PendingUpload;
}

void Model::OnLoad(AssetTracker& anAssetTracker)
{
	ASSERT_STR(myState == State::Invalid, "Double load detected!");

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	constexpr StaticString kDir = Resource::AssetsFolder + "objects/";
	std::string fullName = kDir.CStr() + myPath;
	std::string err;
	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullName.c_str(), kDir.CStr());
	if (!loaded)
	{
		SetErrMsg(move(err));
		return;
	}

	if (!err.empty())
	{
		printf("[Warning] Model loading warning: %s\n", err.c_str());
	}

	myVertices.reserve(attrib.vertices.size());
	std::unordered_map<Vertex, IndexType> uniqueVerts;
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
			if (uniqueVerts.count(vertex) == 0)
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
				size_t currIndex = myVertices.size();
				ASSERT_STR(currIndex < std::numeric_limits<IndexType>::max(), "Vertex index overflow!");
				uniqueVerts[vertex] = static_cast<IndexType>(currIndex); // marking that new vertex is at this index
				myVertices.push_back(vertex); // adding it at the marked position
			}

			// reusing the vertex
			myIndices.push_back(uniqueVerts[vertex]);
		}
	}
	myCenter = (myAABBMin + myAABBMax) / 2.f;
	mySphereRadius = glm::sqrt(mySphereRadius);

	myState = State::PendingUpload;
}

void Model::OnUpload(GPUResource* aGPURes)
{
	myGPUResource = aGPURes;

	CreateDescriptor createDesc;
	createDesc.myPrimitiveType = GPUResource::Primitive::Triangles;
	createDesc.myUsage = GPUResource::Usage::Static;
	createDesc.myVertType = Vertex::Type;
	createDesc.myIsIndexed = true;
	myGPUResource->Create(createDesc);
	
	UploadDescriptor uploadDesc;
	uploadDesc.myPrimitiveCount = myIndices.size();
	uploadDesc.myVertices = myVertices.data();
	uploadDesc.myVertCount = myVertices.size();
	uploadDesc.myIndices = myIndices.data();
	uploadDesc.myIndCount = myIndices.size();
	uploadDesc.myNextDesc = nullptr; // upload everything in one go
	myGPUResource->Upload(uploadDesc);

	myState = State::Ready;
}