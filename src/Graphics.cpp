#include "Graphics.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Graphics* Graphics::activeGraphics = NULL;
ModelId Graphics::currModel;
ShaderId Graphics::currShader;
TextureId Graphics::currTexture;

int Graphics::GetRenderCalls() const
{
	int callsTotal = 0;
	for (int i = 0; i < maxThreads; i++)
		callsTotal += renderCalls[i];
	return callsTotal;
}

void Graphics::ResetRenderCalls()
{
	for (int i = 0; i < maxThreads; i++)
		renderCalls[i] = 0;
}

void Graphics::LoadModel(string name, vector<Vertex> &vertices, vector<uint32_t> &indices, vec3 &center, float &radius)
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;

	string fullName = "assets/objects/" + name + ".obj";
	string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullName.c_str());
	if (!err.empty() || !ret)
	{
		printf("[Error] Failed to load %s: %s\n", name, err);
		return;
	}

	vertices.reserve(vertices.size() + attrib.vertices.size());
	vec3 newCenter;
	float maxLen = 0;
	size_t vertsAdded = 0;
	unordered_map<Vertex, uint> uniqueVerts;
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex;
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			newCenter += vertex.pos;

			if (index.texcoord_index != -1)
			{
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1 - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (index.normal_index != -1)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			// checking for vertex duplication
			if (uniqueVerts.count(vertex) == 0)
			{
				vertsAdded++;
				uniqueVerts[vertex] = vertices.size(); // marking that new vertex is at this index
				vertices.push_back(vertex); // adding it at the marked position

				float len = length(vertex.pos);
				if (len > maxLen)
					maxLen = len;
			}
			
			// reusing the vertex
			indices.push_back(uniqueVerts[vertex]);
		}
	}
	newCenter /= vertsAdded;
	center = newCenter;
	radius = maxLen;
}

unsigned char* Graphics::LoadTexture(string name, int *x, int *y, int *channels, int desiredChannels)
{
	unsigned char* pixels = stbi_load(name.c_str(), x, y, channels, desiredChannels);
	if (pixels == nullptr)
		printf("[Error] Failed to load texture '%s'\n", name.c_str());
	return pixels;
}

void Graphics::FreeTexture(void *data)
{
	stbi_image_free(data);
}

string Graphics::readFile(const string & filename)
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