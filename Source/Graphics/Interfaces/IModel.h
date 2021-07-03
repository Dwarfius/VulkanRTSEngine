#pragma once

#include <Core/Vertex.h>

class IModel
{
public:
	using IndexType = uint32_t;

	enum class PrimitiveType
	{
		Lines,
		Triangles
	};
	constexpr static const char* const kPrimitiveTypeNames[]
	{
		"Lines",
		"Triangles"
	};

	virtual glm::vec3 GetCenter() const = 0;
	virtual float GetSphereRadius() const = 0;
};