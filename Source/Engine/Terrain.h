#pragma once

#include "Vertex.h"

class PhysicsEntity;
class PhysicsShapeHeightfield;

class Terrain
{
public:
	Terrain();
	~Terrain();

	void Generate(string aName, float aStep, float anYScale, float anUvScale);
	
	vector<Vertex>::const_iterator GetVertBegin() const { return myVerts.begin(); }
	vector<Vertex>::const_iterator GetVertEnd() const { return myVerts.end(); }
	vector<uint32_t>::const_iterator GetIndBegin() const { return myIndices.begin(); }
	vector<uint32_t>::const_iterator GetIndEnd() const { return myIndices.end(); }

	glm::vec3 GetCenter() const { return myCenter; }
	float GetRange() const { return myRange; }
	float GetHeight(glm::vec3 pos) const;
	glm::vec3 GetNormal(glm::vec3 pos) const;

	// TODO: should probably remove this now that bullet is in progress
	bool Collides(glm::vec3 aPos, float aRange) const;

	// TODO: need to remove it from terrain class
	void CreatePhysics();
	PhysicsEntity* GetPhysicsEntity() const { return myPhysicsEntity; }

private:
	vector<Vertex> myVerts;
	vector<uint32_t> myIndices;

	int myWidth, myHeight;
	glm::vec3 myStart, myEnd;
	glm::vec3 myCenter;
	float myRange, myStep;

	void Normalize();

	// wraps the val value around [0;range] range
	float Wrap(float aVal, float aRange) const;

	// physics related
	vector<float> myHeightsCache;
	PhysicsShapeHeightfield* myShape;
	PhysicsEntity* myPhysicsEntity;
};