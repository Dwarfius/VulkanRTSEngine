#pragma once

#include "../GPUResource.h"
#include "../Interfaces/IModel.h"

class GPUModel : public GPUResource, public IModel
{
public:
	glm::vec3 GetCenter() const override final { return myCenter; }
	float GetSphereRadius() const override final { return myRadius; }
	uint32_t GetPrimitiveCount() const { return myPrimitiveCount; }

	std::string_view GetTypeName() const final { return "Model"; }

protected:
	glm::vec3 myCenter;
	float myRadius;
	uint32_t myPrimitiveCount;
};