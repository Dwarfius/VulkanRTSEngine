#pragma once

#include <Core/Vertex.h>

enum class PrimitiveType
{
	Lines,
	Triangles
};

class IModel
{
public:
	using IndexType = uint32_t;

	virtual glm::vec3 GetCenter() const = 0;
	virtual float GetSphereRadius() const = 0;
};