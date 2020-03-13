#include "Precomp.h"
#include "EditorMode.h"

#include "../Game.h"
#include "../Input.h"

#include <Graphics/Camera.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsShapes.h>

#include "PhysicsComponent.h"
#include "GameObject.h"
#include "VisualObject.h"

EditorMode::EditorMode(PhysicsWorld& aWorld)
	: myMouseSensitivity(0.1f)
	, myFlightSpeed(2.f)
{
	myPhysShape = std::make_shared<PhysicsShapeBox>(glm::vec3(0.5f));
}

void EditorMode::Update(float aDeltaTime, PhysicsWorld& aWorld)
{
	// FPS camera implementation
	Camera* cam = Game::GetInstance()->GetCamera();
	Transform& camTransf = cam->GetTransform();

	glm::vec3 pos = camTransf.GetPos();
	glm::vec3 forward = camTransf.GetForward();
	glm::vec3 right = camTransf.GetRight();
	glm::vec3 up = camTransf.GetUp();
	if (Input::GetKey('W'))
	{
		pos += forward * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('S'))
	{
		pos -= forward * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('D'))
	{
		pos += right * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('A'))
	{
		pos -= right * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('R'))
	{
		pos += up * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('F'))
	{
		pos -= up * myFlightSpeed * aDeltaTime;
	}
	camTransf.SetPos(pos);

	// General controls
	if (Input::GetKeyPressed('K'))
	{
		Graphics::ourUseWireframe = !Graphics::ourUseWireframe;
	}

	if (Input::GetKeyPressed('U'))
	{
		myFlightSpeed *= 2;
	}
	if (Input::GetKeyPressed('J'))
	{
		myFlightSpeed /= 2;
	}

	// to avoid accumulating roll from quaternion applications, have to apply then separately
	const glm::vec2 mouseDelta = Input::GetMouseDelta();
	
	glm::vec3 pitchDelta(-mouseDelta.y, 0.f, 0.f);
	pitchDelta *= myMouseSensitivity;
	const glm::quat pitchRot(glm::radians(pitchDelta));

	glm::vec3 yawDelta(0.f, -mouseDelta.x, 0.f);
	yawDelta *= myMouseSensitivity;
	const glm::quat yawRot(glm::radians(yawDelta));

	camTransf.SetRotation(yawRot * camTransf.GetRotation() * pitchRot);

	if (Input::GetMouseBtnPressed(0))
	{
		glm::vec3 from = camTransf.GetPos();
		glm::vec3 dir = camTransf.GetForward();

		PhysicsEntity* physEntity;
		if (aWorld.RaycastClosest(from, dir, 100.f, physEntity))
		{
			physEntity->AddForce(dir * 100.f);
		}
	}

	if (Input::GetMouseBtnPressed(1))
	{
		GameObject* go = Game::GetInstance()->Instantiate(glm::vec3(0));
		{
			PhysicsComponent* physComp = go->AddComponent<PhysicsComponent>();
			
			AssetTracker& assetTracker = Game::GetInstance()->GetAssetTracker();
			VisualObject* vo = new VisualObject(*go);
			Handle<Model> cubeModel = assetTracker.GetOrCreate<Model>("cube.obj", [physComp, this, &aWorld](const Resource* aRes) {
				const Model* model = static_cast<const Model*>(aRes);
				physComp->SetOrigin(model->GetCenter());
				physComp->CreatePhysicsEntity(0, myPhysShape);
				physComp->RequestAddToWorld(aWorld);
			});
			vo->SetModel(cubeModel);
			vo->SetPipeline(assetTracker.GetOrCreate<Pipeline>("default.ppl"));
			vo->SetTexture(assetTracker.GetOrCreate<Texture>("CubeUnwrap.jpg"));
			go->SetVisualObject(vo);
		}
	}
}