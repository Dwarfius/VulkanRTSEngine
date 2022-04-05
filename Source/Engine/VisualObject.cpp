#include "Precomp.h"
#include "VisualObject.h"

#include "Game.h"

#include <Graphics/GPUResource.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/UniformBuffer.h>

void VisualObject::SetModel(Handle<Model> aModel)
{
	myModel = Game::GetInstance()->GetGraphics()->GetOrCreate(aModel).Get<GPUModel>();
}

void VisualObject::SetPipeline(Handle<Pipeline> aPipeline)
{
	myPipeline = Game::GetInstance()->GetGraphics()->GetOrCreate(aPipeline).Get<GPUPipeline>();
	myIsResolved = false;
	myIsNewPipeline = true;
}

void VisualObject::SetTexture(Handle<Texture> aTexture)
{
	myTexture = Game::GetInstance()->GetGraphics()->GetOrCreate(aTexture).Get<GPUTexture>();
}

glm::vec3 VisualObject::GetCenter() const
{
	const glm::vec3 pos = myTransf.GetPos();
	const glm::vec3 scale = myTransf.GetScale();
	const glm::vec3 localCenter = myModel->GetCenter();
	return pos + scale * localCenter;
}

float VisualObject::GetRadius() const
{
	const glm::vec3 scale = myTransf.GetScale();
	const float maxScale = std::max({ scale.x, scale.y, scale.z });
	const float radius = myModel->GetSphereRadius();
	return maxScale * radius;
}

bool VisualObject::Resolve()
{
	// TODO: get another look at this, I think it can be resolved
	// via Pipeline and ExecLambdaOnLoad
	if (myPipeline->GetState() != GPUResource::State::Valid
		|| std::any_of(myUniforms.begin(), myUniforms.end(), 
			[](const Handle<UniformBuffer>& aBuffer)
				{ return aBuffer->GetState() != GPUResource::State::Valid; }
			)
		)
	{
		return false;
	}

	if (myIsNewPipeline)
	{
		// Since we got a new pipeline, time to replace
		// descriptors, UBOs and adapters
		myUniforms.clear();

		Graphics& graphics = *Game::GetInstance()->GetGraphics();
		const GPUPipeline* pipeline = myPipeline.Get();
		size_t descriptorCount = pipeline->GetAdapterCount();
		for (size_t i = 0; i < descriptorCount; i++)
		{
			const UniformAdapter& adapter = pipeline->GetAdapter(i);
			const size_t bufferSize = adapter.GetDescriptor().GetBlockSize();
			Handle<UniformBuffer> buffer = graphics.CreateUniformBuffer(bufferSize);
			myUniforms.push_back(buffer);
		}
		myIsNewPipeline = false;
		return false;
	}

	myIsResolved = true;
	return myIsResolved;
}