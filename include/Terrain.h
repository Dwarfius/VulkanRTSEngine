#ifndef _TERRAIN_H
#define _TERRAIN_H

#include "Common.h"
#include "Vertex.h"

class Terrain
{
public:
	void Generate(string name, float step, vec3 offest, float yScale, float uvScale);
	
	auto GetVertBegin() { return verts.begin(); }
	auto GetVertEnd() { return verts.end(); }
	auto GetIndBegin() { return indices.begin(); }
	auto GetIndEnd() { return indices.end(); }

	vec3 GetCenter() { return center; }
	float GetRange() { return range; }
	float GetHeight(vec3 pos);
	vec3 GetNormal(vec3 pos);

	bool Collides(vec3 pos, float range);

private:
	vector<Vertex> verts;
	vector<uint32_t> indices;

	int width, height;
	vec3 start, end;
	vec3 center;
	float range, step;

	void Normalize();

	// wraps the val value around [0;range] range
	float Wrap(float val, float range);
};
#endif // !_TERRAIN_H
