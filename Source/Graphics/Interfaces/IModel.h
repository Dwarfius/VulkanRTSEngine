#pragma once

#include <Core/Vertex.h>

enum class PrimitiveType
{
	Lines,
	Triangles
};

// TODO: template the Model on vertex and index type
class IModel
{
public:
	using VertexType = Vertex;
	using IndexType = uint32_t;

	virtual glm::vec3 GetCenter() const = 0;
	virtual float GetSphereRadius() const = 0;
};