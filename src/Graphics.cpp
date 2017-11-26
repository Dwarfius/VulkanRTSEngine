#include "Common.h"
#include "Graphics.h"
#include "Camera.h"

Graphics* Graphics::activeGraphics = NULL;
ModelId Graphics::currModel;
ShaderId Graphics::currShader;
TextureId Graphics::currTexture;
int Graphics::width = 800;
int Graphics::height = 600;

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

	const string baseDir = "assets/objects/";
	string fullName = baseDir + name + ".obj";
	string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullName.c_str(), baseDir.c_str());
	if (!ret)
	{
		printf("[Error] Failed to load %s: %s\n", name.c_str(), err.c_str());
		return;
	}
	if (!err.empty())
		printf("[Warning] Model loading warning: %s\n", err.c_str());

	vertices.reserve(vertices.size() + attrib.vertices.size());
	vec3 min, max;
	float maxLen = 0;
	unordered_map<Vertex, uint32_t> uniqueVerts;
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex;
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

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
				if (vertex.pos.x < min.x)
					min.x = vertex.pos.x;
				if (vertex.pos.y < min.y)
					min.y = vertex.pos.y;
				if (vertex.pos.z < min.z)
					min.z = vertex.pos.z;

				if (vertex.pos.x > max.x)
					max.x = vertex.pos.x;
				if (vertex.pos.y > max.y)
					max.y = vertex.pos.y;
				if (vertex.pos.z > max.z)
					max.z = vertex.pos.z;

				uniqueVerts[vertex] = static_cast<uint32_t>(vertices.size()); // marking that new vertex is at this index
				vertices.push_back(vertex); // adding it at the marked position

				float len = length(vertex.pos);
				if (len > maxLen)
					maxLen = len;
			}
			
			// reusing the vertex
			indices.push_back(uniqueVerts[vertex]);
		}
	}
	//printf("[Info] Min: %f, %f, %f; Max: %f, %f, %f\n", min.x, min.y, min.z, max.x, max.y, max.z);
	center = (max + min) / 2.f;
	//printf("[Info] Center was %f, %f, %f for %s\n", center.x, center.y, center.z, name.c_str());
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