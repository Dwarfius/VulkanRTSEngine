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

private:
	vector<Vertex> verts;
	vector<uint32_t> indices;

	vec3 center;
	float range;
};
#endif // !_TERRAIN_H
