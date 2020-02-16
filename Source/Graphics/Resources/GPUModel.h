#pragma once

#include "../GPUResource.h"
#include "../Interfaces/IModel.h"

class GPUModel : public GPUResource, public IModel
{
public:
	glm::vec3 GetCenter() const override final { return myCenter; }
	float GetSphereRadius() const override final { return myRadius; }

protected:
	glm::vec3 myCenter;
	float myRadius;
};