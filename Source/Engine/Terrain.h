#pragma once

#include "Vertex.h"

class PhysicsEntity;
class PhysicsShapeHeightfield;

class Terrain
{
	typedef vector<Vertex> VertVector;
	typedef vector<IndexType> IndxVector;

	typedef VertVector::const_iterator VertIterator;
	typedef IndxVector::const_iterator IndxIterator;

public:
	void Generate(string aName, float aStep, float anYScale, float anUvScale);
	
	VertIterator GetVertBegin() const { return myVerts.begin(); }
	VertIterator GetVertEnd() const { return myVerts.end(); }
	IndxIterator GetIndBegin() const { return myIndices.begin(); }
	IndxIterator GetIndEnd() const { return myIndices.end(); }

	glm::vec3 GetCenter() const { return myCenter; }
	float GetRange() const { return myRange; }
	float GetHeight(glm::vec3 pos) const;
	glm::vec3 GetNormal(glm::vec3 pos) const;

	// TODO: need to remove it from terrain class
	shared_ptr<PhysicsEntity> CreatePhysics();

private:
	VertVector myVerts;
	IndxVector myIndices;

	int myWidth, myHeight;
	glm::vec3 myStart, myEnd;
	glm::vec3 myCenter;
	float myRange, myStep;

	void Normalize();

	// wraps the val value around [0;range] range
	float Wrap(float aVal, float aRange) const;

	// Preserve the heights so that physics can still reference to it
	vector<float> myHeightsCache;
};