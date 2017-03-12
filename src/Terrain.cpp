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
	float height = BilinearInterp(v0.pos.y, v1.pos.y, v2.pos.y, v3.pos.y, x, y);
	printf("[Info] %f from (%f, %f, %f, %f) for (%f, %f)\n", height, v0.pos.y, v1.pos.y, v2.pos.y, v3.pos.y, x, y);
	return height;
}

// f0=f(0,0), f1=f(0,1), f2=f(1,0), f3=f(1,1) 
float Terrain::BilinearInterp(float f0, float f1, float f2, float f3, float x, float y)
{
	return f0 * (1 - x) * (1 - y) +
		f1 * x * (1 - y) +
		f2 * (1 - x) * y +
		f3 * x * y;
}