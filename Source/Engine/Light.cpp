#include "Precomp.h"
#include "Light.h"

#include "Game.h"

#include <Graphics/UniformAdapterRegister.h>

void LightAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const LightSystem& system = Game::GetInstance()->GetLightSystem();
	uint32_t accumulated = 0;
	system.ForEach([&](const Light& aLight) {
		if (accumulated == kMaxLights)
		{
			return;
		}
		
		const glm::vec4 posRange { 
			aLight.myTransform.GetPos(), 
			aLight.myAmbientIntensity 
		};
		aUB.SetUniform(0, accumulated, posRange);
		const glm::quat rot = aLight.myTransform.GetRotation();
		const glm::vec4 rotAsVec{ rot.x, rot.y, rot.z, rot.w };
		aUB.SetUniform(1, accumulated, rot);
		const glm::vec4 colorPower{ aLight.myColor, aLight.myType };
		aUB.SetUniform(2, accumulated, colorPower);
		aUB.SetUniform(3, accumulated, aLight.myAttenuation);
		accumulated++;
	});
	aUB.SetUniform(4, 0, accumulated);
}