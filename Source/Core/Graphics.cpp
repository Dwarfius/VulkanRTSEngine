#include "Precomp.h"
#include "Graphics.h"
#include "Camera.h"

#include <limits>

Graphics* Graphics::ourActiveGraphics = NULL;
int Graphics::ourWidth = 800;
int Graphics::ourHeight = 600;

void Graphics::LoadModel(string aName, vector<Vertex>& aVertices, vector<IndexType>& anIndices, glm::vec3& aCenter, float& aRadius)
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;

	const string baseDir = "assets/objects/";
	string fullName = baseDir + aName + ".obj";
	string err;
	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullName.c_str(), baseDir.c_str());
	if (!loaded)
	{
		printf("[Error] Failed to load %s: %s\n", aName.c_str(), err.c_str());
		return;
	}
	if (!err.empty())
	{
		printf("[Warning] Model loading warning: %s\n", err.c_str());
	}

	aVertices.reserve(aVertices.size() + attrib.vertices.size());
	glm::vec3 min, max;
	float maxLen = 0;
	unordered_map<Vertex, IndexType> uniqueVerts;
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
				min.x = glm::min(min.x, vertex.myPos.x);
				min.y = glm::min(min.y, vertex.myPos.y);
				min.z = glm::min(min.z, vertex.myPos.z);

				max.x = glm::max(max.x, vertex.myPos.x);
				max.y = glm::max(max.y, vertex.myPos.y);
				max.z = glm::max(max.z, vertex.myPos.z);

				// update radius by finding the furthest away vert
				maxLen = glm::max(maxLen, glm::length(vertex.myPos));

				// push back the new vertex and record it's position
				size_t currIndex = aVertices.size();
				assert(currIndex < numeric_limits<IndexType>::max()); // just to make sure it won't get forgotten
				IndexType currCastedIndex = static_cast<IndexType>(currIndex);
				uniqueVerts[vertex] = currCastedIndex; // marking that new vertex is at this index
				aVertices.push_back(vertex); // adding it at the marked position
			}
			
			// reusing the vertex
			anIndices.push_back(uniqueVerts[vertex]);
		}
	}
	aCenter = (max + min) / 2.f;
	aRadius = maxLen;
}

unsigned char* Graphics::LoadTexture(string aName, int* aWidth, int* aHeight, int* aChannels, int aDesiredChannels)
{
	unsigned char* pixels = stbi_load(aName.c_str(), aWidth, aHeight, aChannels, aDesiredChannels);
	if (!pixels)
	{
		printf("[Error] Failed to load texture '%s'\n", aName.c_str());
	}
	return pixels;
}

void Graphics::FreeTexture(void *data)
{
	stbi_image_free(data);
}

string Graphics::ReadFile(const string & filename) const
{
	// opening at the end allows us to know size quickly
	ifstream file(filename, ios::ate | ios::binary);
	if (!file.is_open())
	{
		printf("[Error] Failed to open file: %s\n", filename.c_str());
		return "";
	}

	size_t size = file.tellg();
	string data;
	data.resize(size);
	file.seekg(0);
	file.read(&data[0], size);
	file.close();
	return data;
}