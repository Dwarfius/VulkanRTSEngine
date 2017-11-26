#include "Common.h"
#include "Terrain.h"
#include "Graphics.h"

void Terrain::Generate(string name, float step, vec3 offset, float yScale, float uvScale)
{
	this->step = step;
	unsigned char* pixels;
	int channels;
	pixels = Graphics::LoadTexture(name, &width, &height, &channels, 1);

	//first, creating the vertices
	start = offset;
	end = start + vec3((width - 1) * step, 0, (height - 1) * step);
	verts.reserve(height * width);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Vertex v;
			v.pos.x = x * step + offset.x;
			v.pos.z = y * step + offset.z;
			v.pos.y = pixels[y * width + x] / 255.f * yScale + offset.y;
			v.uv.x = Wrap(static_cast<float>(x), uvScale);
			v.uv.y = Wrap(static_cast<float>(y), uvScale);
			verts.push_back(v);
		}
	}

	center = offset + (end - start) / 2.f;
	range = static_cast<float>(sqrt(width * width + height * height));

	//now creating indices
	indices.reserve((height - 1) * (width - 1) * 6);
	for (int y = 0; y < height - 1; y++)
	{
		for (int x = 0; x < width - 1; x++)
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

float Terrain::GetHeight(vec3 pos) const
{
	// finding the relative position
	// + center cause rendering uses center anchors
	float x = (pos.x + center.x) / step;
	float y = (pos.z + center.z) / step;

	// just to be safe - it's clamped to the edges
	x = clamp(x, 0.f, width - 2.f);
	y = clamp(y, 0.f, height - 2.f);

	// printf("[Info] %f %f for (%f, %f), where start (%f, %f)\n", x, y, pos.x, pos.z, center.x, center.z);

	// getting the actual vert indices
	int xMin = static_cast<int>(floor(x));
	int xMax = xMin + 1;
	int yMin = static_cast<int>(floor(y));
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
	return mix(botHeight, topHeight, y);
}

vec3 Terrain::GetNormal(vec3 pos) const
{
	// finding the relative position
	// + center cause rendering uses center anchors
	float x = (pos.x + center.x) / step;
	float y = (pos.z + center.z) / step;

	// just to be safe - it's clamped to the edges
	x = clamp(x, 0.f, width - 2.f);
	y = clamp(y, 0.f, height - 2.f);

	// printf("[Info] %f %f for (%f, %f), where start (%f, %f)\n", x, y, pos.x, pos.z, center.x, center.z);

	// getting the actual vert indices
	int xMin = static_cast<int>(floor(x));
	int xMax = xMin + 1;
	int yMin = static_cast<int>(floor(y));
	int yMax = yMin + 1;

	// getting vertices for lerping
	Vertex v0 = verts[yMin * width + xMin];
	Vertex v1 = verts[yMax * width + xMin];
	Vertex v2 = verts[yMin * width + xMax];
	Vertex v3 = verts[yMax * width + xMax];

	// getting the normal
	x -= xMin;
	y -= yMin;
	vec3 botNorm = mix(v0.normal, v2.normal, x);
	vec3 topNorm = mix(v1.normal, v3.normal, x);
	return mix(botNorm, topNorm, y);
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

float Terrain::Wrap(float val, float range) const
{
	float cosA = cos((val / range) * pi<float>() / 2);
	float normalized = (cosA + 1) / 2;
	return normalized;
}

bool Terrain::Collides(vec3 pos, float range) const
{
	float height = GetHeight(pos);
	return height > pos.y - range;
}