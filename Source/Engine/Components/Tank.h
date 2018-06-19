#pragma once

#include "ComponentBase.h"

class Tank : public ComponentBase
{
public:
	Tank(bool aTeam);

	void Update(float aDeltaTime) override;
	void Destroy() override;

	void SetOnDeathCallback(function<void(ComponentBase*)> aCallback) { myOnDeathCallback = aCallback; }
	void SetNavTarget(glm::vec3 aTarget) { myNavTarget = aTarget; }
	bool GetTeam() const { return myTeam; }

	int GetComponentType() override { return Type; }
	const static int Type;

private:
	glm::vec3 myNavTarget;
	float myShootTimer;
	float myShootRate;
	bool myTeam;

	function<void(ComponentBase*)> myOnDeathCallback;
};