#pragma once

#include "Vertex.h"

class Terrain
{
public:
	void Generate(string name, float step, vec3 offest, float yScale, float uvScale);
	
	auto GetVertBegin() const { return verts.begin(); }
	auto GetVertEnd() const { return verts.end(); }
	auto GetIndBegin() const { return indices.begin(); }
	auto GetIndEnd() const { return indices.end(); }

	vec3 GetCenter() const { return center; }
	float GetRange() const { return range; }
	float GetHeight(vec3 pos) const;
	vec3 GetNormal(vec3 pos) const;

	bool Collides(vec3 pos, float range) const;

private:
	vector<Vertex> verts;
	vector<uint32_t> indices;

	int width, height;
	vec3 start, end;
	vec3 center;
	float range, step;

	void Normalize();

	// wraps the val value around [0;range] range
	float Wrap(float val, float range) const;
};