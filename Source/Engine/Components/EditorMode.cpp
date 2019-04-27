#include "Precomp.h"
#include "EditorMode.h"

#include "../Game.h"
#include "../Input.h"

#include <Core/Camera.h>

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsShapes.h>

#include "PhysicsComponent.h"
#include "GameObject.h"
#include "VisualObject.h"

EditorMode::EditorMode(PhysicsWorld& aWorld)
{
	shared_ptr<PhysicsShapeBase> sphereShape = make_shared<PhysicsShapeSphere>(1.f);
	shared_ptr<PhysicsShapeBox> boxShape = make_shared<PhysicsShapeBox>(glm::vec3(0.5f));

	// a sphere with no visual object (don't have a mesh atm)
	GameObject* go = Game::GetInstance()->Instantiate(glm::vec3(0, 5, 0));
	PhysicsEntity* physEntity = new PhysicsEntity(1.f, sphereShape, *go);
	{
		PhysicsComponent* physComp = new PhysicsComponent();
		physComp->SetPhysicsEntity(physEntity);
		physComp->RequestAddToWorld(aWorld);
		go->AddComponent(physComp);
	}
	myBall = go;

	// a cube with a visual object
	go = Game::GetInstance()->Instantiate(glm::vec3(2, 5, 0));
	physEntity = new PhysicsEntity(1.f, boxShape, *go);
	{
		PhysicsComponent* physComp = new PhysicsComponent();
		physComp->SetPhysicsEntity(physEntity);
		physComp->RequestAddToWorld(aWorld);
		go->AddComponent(physComp);

		AssetTracker& assetTracker = Game::GetInstance()->GetAssetTracker();
		VisualObject* vo = new VisualObject(*go);
		vo->SetModel(assetTracker.GetOrCreate<Model>("cube.obj"));
		vo->SetPipeline(assetTracker.GetOrCreate<Pipeline>("default.ppl"));
		vo->SetTexture(assetTracker.GetOrCreate<Texture>("CubeUnwrap.jpg"));
		go->SetVisualObject(vo);
	}
}

EditorMode::~EditorMode()
{
}

void EditorMode::Update(float aDeltaTime, const PhysicsWorld& aWorld) const
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

	// Because the ball doesn't have a visual object attached,
	// have to draw out something in order to verify it's where
	// it should be
	pos = myBall->GetTransform().GetPos();
	right = myBall->GetTransform().GetRight();
	up = myBall->GetTransform().GetUp();
	forward = myBall->GetTransform().GetForward();
	Game::GetInstance()->AddLine(pos - right, pos + right, glm::vec3(1, 0, 0));
	Game::GetInstance()->AddLine(pos - up, pos + up, glm::vec3(1, 0, 0));
	Game::GetInstance()->AddLine(pos - forward, pos + forward, glm::vec3(1, 0, 0));
}