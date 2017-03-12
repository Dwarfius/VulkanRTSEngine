#ifndef _TERRAIN_H
#define _TERRAIN_H

#include "Common.h"
#include "Vertex.h"

class Terrain
{
public:
	void Generate(string name, float step, vec3 offest, float yScale);
	
	auto GetVertBegin() { return verts.begin(); }
	auto GetVertEnd() { return verts.end(); }
	auto GetIndBegin() { return indices.begin(); }
	auto GetIndEnd() { return indices.end(); }

	vec3 GetCenter() { return center; }
	float GetRange() { return range; }
	float GetHeight(vec3 pos);

private:
	vector<Vertex> verts;
	vector<uint32_t> indices;

	int width, height;
	vec3 start, end;
	vec3 center;
	float range, step;

	float BilinearInterp(float f0, float f1, float f2, float f3, float x, float y);
};
#endif // !_TERRAIN_H
