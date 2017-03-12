#include "Terrain.h"
#include "Graphics.h"

void Terrain::Generate(string name, float step, vec3 offset, float yScale)
{
	this->step = step;
	int channels;
	unsigned char *pixels = Graphics::LoadTexture(name, &width, &height, &channels, 1);

	//first, creating the vertices
	start = offset;
	end = start + vec3((width - 1) * step, 0, (height - 1) * step);
	verts.reserve(height * width);
	for (uint32_t y = 0; y < height; y++)
	{
		for (uint32_t x = 0; x < width; x++)
		{
			Vertex v;
			v.pos.x = x * step + offset.x;
			v.pos.z = y * step + offset.z;
			v.pos.y = pixels[y * width + x] / 255.f * yScale + offset.y;
			v.uv.x = x % 2;
			v.uv.y = y % 2;
			verts.push_back(v);
		}
	}

	center = offset + (end - start) / 2.f;
	range = sqrt(width * width + height * height);

	//now creating indices
	indices.reserve((height - 1) * (width - 1) * 6);
	for (uint32_t y = 0; y < height - 1; y++)
	{
		for (uint32_t x = 0; x < width - 1; x++)
		{
			uint32_t tl = y * width + x;
			uint32_t tr = tl + 1;
			uint32_t bl = tl + width;
			uint32_t br = bl + 1;
			indices.push_back(tl);
			indices.push_back(bl);
			indices.push_back(tr);
			indices.push_back(br);
			indices.push_back(tr);
			indices.push_back(bl);
		}
	}

	Normalize();
}

float Terrain::GetHeight(vec3 pos)
{
	// finding the relative position
	// + center cause rendering uses center anchors
	float x = (pos.x + center.x) / step;
	float y = (pos.z + center.z) / step;

	// just to be safe - it's clamped to the edges
	x = clamp(x, 0.f, width - 1.f);
	y = clamp(y, 0.f, height - 1.f);

	// printf("[Info] %f %f for (%f, %f), where start (%f, %f)\n", x, y, pos.x, pos.z, center.x, center.z);

	// getting the actual vert indices
	int xMin = floor(x);
	int xMax = xMin + 1;
	int yMin = floor(y);
	int yMax = yMin + 1;

	// getting vertices for lerping
	Vertex v0 = verts[yMin * width + xMin];
	Vertex v1 = verts[yMax * width + xMin];
	Vertex v2 = verts[yMin * width + xMax];
	Vertex v3 = verts[yMax * width + xMax];

	// getting the height
	x -= xMin;
	y -= yMin;
	float botHeight = mix(v0.pos.y, v2.pos.y, x);
	float topHeight = mix(v1.pos.y, v3.pos.y, x);
	float height = mix(botHeight, topHeight, y);
	printf("[Info] %f from (%f, %f, %f, %f) for (%f, %f)\n", height, v0.pos.y, v1.pos.y, v2.pos.y, v3.pos.y, x, y);
	return height;
}

vec3 Terrain::GetNormal(vec3 pos)
{
	// finding the relative position
	// + center cause rendering uses center anchors
	float x = (pos.x + center.x) / step;
	float y = (pos.z + center.z) / step;

	// just to be safe - it's clamped to the edges
	x = clamp(x, 0.f, width - 1.f);
	y = clamp(y, 0.f, height - 1.f);

	// printf("[Info] %f %f for (%f, %f), where start (%f, %f)\n", x, y, pos.x, pos.z, center.x, center.z);

	// getting the actual vert indices
	int xMin = floor(x);
	int xMax = xMin + 1;
	int yMin = floor(y);
	int yMax = yMin + 1;

	// getting vertices for lerping
	Vertex v0 = verts[yMin * width + xMin];
	Vertex v1 = verts[yMax * width + xMin];
	Vertex v2 = verts[yMin * width + xMax];
	Vertex v3 = verts[yMax * width + xMax];

	return vec3();
}

void Terrain::Normalize()
{
	//holds the sum of all surface normals per vertex
	vector<vec3> surfNormals(indices.size(), vec3());
	//gotta update the faces
	for (int i = 0; i < indices.size(); i += 3)
	{
		int i1 = indices.at(i);
		vec3 v1 = verts.at(i1).pos;
		int i2 = indices.at(i + 1);
		vec3 v2 = verts.at(i2).pos;
		int i3 = indices.at(i + 2);
		vec3 v3 = verts.at(i3).pos;

		//calculating the surf normal
		vec3 u = v2 - v1;
		vec3 v = v3 - v1;

		vec3 normal;
		normal.x = u.y * v.z - u.z * v.y;
		normal.y = u.z * v.x - u.x * v.z;
		normal.z = u.x * v.y - u.y * v.x;

		surfNormals[i1] += normal;
		surfNormals[i2] += normal;
		surfNormals[i3] += normal;
	}

	for (int vertInd = 0; vertInd < verts.size(); vertInd++)
		verts.at(vertInd).normal = normalize(surfNormals[vertInd]);
}