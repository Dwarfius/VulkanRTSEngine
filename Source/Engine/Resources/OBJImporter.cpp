#include "Precomp.h"
#include "OBJImporter.h"

#include <Graphics/Resources/Model.h>
#include <Core/File.h>
#include <TinyObjLoader/tiny_obj_loader.h>
#include <sstream>

bool OBJImporter::Load(const std::string& aPath)
{
	File file(aPath);
	return file.Read() && Load(file);
}

bool OBJImporter::Load(const File& aFile)
{
	return Load(aFile.GetBuffer());
}

bool OBJImporter::Load(const std::vector<char>& aBuffer)
{
	// TODO: extend Model to support more than 1 Vertex type loading
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	// Note: this causes a full copy of buffer twice, but it's okay, since 
	// importing external assets is not per-frame frequent
	std::string stringBuffer(aBuffer.cbegin(), aBuffer.cend());
	std::istringstream stream(stringBuffer.c_str());
	tinyobj::MaterialFileReader matFileReader(Resource::kAssetsFolder.CStr());
	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &stream, &matFileReader);
	if (!loaded)
	{
		ASSERT_STR(false, err.c_str());
		return false;
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
	std::vector<Model::IndexType> modelIndices;
	modelIndices.resize(indexCount);

	std::vector<Vertex> modelVertices;
	modelVertices.resize(attrib.vertices.size());

	auto vertEq = [](const Vertex& aLeft, const Vertex& aRight) {
		return aLeft.myPos == aRight.myPos
			&& aLeft.myUv == aRight.myUv
			&& aLeft.myNormal == aRight.myNormal;
	};
	using VertMap = std::unordered_map<Vertex, Model::IndexType, std::hash<Vertex>, decltype(vertEq)>;
	VertMap uniqueVerts(0, std::hash<Vertex>(), vertEq);
	size_t vertsFound = 0;
	indexCount = 0;

	glm::vec3 aabbMin(std::numeric_limits<float>::max());
	glm::vec3 aabbMax(std::numeric_limits<float>::min());
	float sphereRadius = 0.f;

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
				aabbMin = glm::min(aabbMin, vertex.myPos);
				aabbMax = glm::max(aabbMax, vertex.myPos.x);

				// update radius by finding the furthest away vert
				sphereRadius = glm::max(sphereRadius, glm::length2(vertex.myPos));

				// push back the new vertex and record it's position
				ASSERT_STR(vertsFound < std::numeric_limits<Model::IndexType>::max(), "Vertex index overflow!");
				// TODO: this insert with a hint is implementation dependent (might be optimization,
				// might be a slowdown cause bad hint). Need to come back later and reread implementation
				// marking that new vertex is at this index
				iter = uniqueVerts.insert(uniqueVerts.begin(), { vertex, static_cast<Model::IndexType>(vertsFound) });
				modelVertices[vertsFound] = vertex; // adding it at the marked position
				vertsFound++;
			}

			// reusing the vertex
			modelIndices[indexCount++] = iter->second;
		}
	}

	Model::VertStorage<Vertex>* storage = new Model::VertStorage<Vertex>(modelVertices.size());
	myModel = new Model(PrimitiveType::Triangles, storage, true);

	Model::UploadDescriptor<Vertex> uploadDesc;
	uploadDesc.myVertices = modelVertices.data();
	uploadDesc.myVertCount = modelVertices.size();
	uploadDesc.myIndices = modelIndices.data();
	uploadDesc.myIndCount = modelIndices.size();
	uploadDesc.myNextDesc = nullptr;
	uploadDesc.myVertsOwned = false;
	uploadDesc.myIndOwned = false;
	myModel->Update(uploadDesc);

	myModel->SetAABB(aabbMin, aabbMax);
	myModel->SetSphereRadius(sphereRadius);
	return true;
}