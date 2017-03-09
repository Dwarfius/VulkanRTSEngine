#include "Terrain.h"
#include "Graphics.h"

void Terrain::Generate(string name, float step, vec3 offset, float yScale)
{
	int h, w, channels;
	unsigned char *pixels = Graphics::LoadTexture(name, &w, &h, &channels, 1);

	//first, creating the vertices
	verts.reserve(h * w);
	for (uint32_t y = 0; y < h; y++)
	{
		for (uint32_t x = 0; x < w; x++)
		{
			Vertex v;
			v.pos.x = x * step + offset.x;
			v.pos.z = y * step + offset.z;
			v.pos.y = pixels[y * w + x] / 255.f * yScale + offset.y;
			verts.push_back(v);
		}
	}

	center = offset + vec3(w / 2 * step, 0, h / 2 * step);
	range = sqrt(w * w + h * h);

	//now creating indices
	indices.reserve((h - 1) * (w - 1) * 6);
	for (uint32_t y = 0; y < h - 1; y++)
	{
		for (uint32_t x = 0; x < w - 1; x++)
		{
			uint32_t tl = y * w + x;
			uint32_t tr = tl + 1;
			uint32_t bl = tl + w;
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