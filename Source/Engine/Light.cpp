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
		aUB.SetUniform(ourDescriptor.GetOffset(0, accumulated), posRange);
		const glm::vec4 lightDirAndInnerLimit{
			aLight.myTransform.GetForward(),
			aLight.mySpotlightLimits[0]
		};
		aUB.SetUniform(ourDescriptor.GetOffset(1, accumulated), lightDirAndInnerLimit);
		const glm::vec4 colorPower{ aLight.myColor, aLight.myType };
		aUB.SetUniform(ourDescriptor.GetOffset(2, accumulated), colorPower);
		const glm::vec4 attenuationAndOuterLimit{
			aLight.myAttenuation,
			aLight.mySpotlightLimits[1]
		};
		aUB.SetUniform(ourDescriptor.GetOffset(3, accumulated), attenuationAndOuterLimit);
		accumulated++;
	});
	aUB.SetUniform(ourDescriptor.GetOffset(4, 0), accumulated);
}