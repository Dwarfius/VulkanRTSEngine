#pragma once

#include <Core/Vertex.h>
#include <Core/DataEnum.h>

class IModel
{
public:
	using IndexType = uint32_t;

	DATA_ENUM(PrimitiveType, char, 
		Lines,
		Triangles
	);

	virtual glm::vec3 GetCenter() const = 0;
	virtual float GetSphereRadius() const = 0;
};