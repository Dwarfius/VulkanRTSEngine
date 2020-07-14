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
	, myDemoWindowVisible(false)
{
	myPhysShape = std::make_shared<PhysicsShapeBox>(glm::vec3(0.5f));
}

void EditorMode::Update(Game& aGame, float aDeltaTime, PhysicsWorld& aWorld)
{
	// FPS camera implementation
	Camera* cam = aGame.GetCamera();
	Transform& camTransf = cam->GetTransform();

	if (Input::GetMouseBtn(1))
	{
		HandleCamera(camTransf, aDeltaTime);
	}

	if (Input::GetKeyPressed(Input::Keys::F1))
	{
		myDemoWindowVisible = !myDemoWindowVisible;
	}
	{
		tbb::mutex::scoped_lock lock(aGame.GetImGUISystem().GetMutex());
		if (myDemoWindowVisible)
		{
			ImGui::ShowDemoWindow(&myDemoWindowVisible);
		}
		else
		{
			ImGui::SetNextWindowPos({ 0,0 });
			ImGui::Begin("Info");
			ImGui::Text("Press F1 to bring the ImGUIDebugWindow");
			ImGui::End();
		}
	}

	if (Input::GetMouseBtnPressed(0))
	{
		glm::vec3 from = camTransf.GetPos();
		glm::vec3 dir = camTransf.GetForward();

		PhysicsEntity* physEntity;
		if (aWorld.RaycastClosest(from, dir, 100.f, physEntity)
			&& !physEntity->IsStatic())
		{
			physEntity->AddForce(dir * 100.f);
		}
	}

	if (Input::GetKeyPressed(Input::Keys::K))
	{
		Graphics::ourUseWireframe = !Graphics::ourUseWireframe;
	}

	if (Input::GetMouseBtnPressed(2))
	{
		GameObject* go = aGame.Instantiate(camTransf.GetPos());
		PhysicsComponent* physComp = go->AddComponent<PhysicsComponent>();
			
		AssetTracker& assetTracker = aGame.GetAssetTracker();
		VisualObject* vo = new VisualObject(*go);

		Handle<Model> cubeModel = assetTracker.GetOrCreate<Model>("cube.obj");
		cubeModel->ExecLambdaOnLoad([physComp, this, &aWorld](const Resource* aRes) {
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

void EditorMode::HandleCamera(Transform& aCamTransf, float aDeltaTime)
{
	glm::vec3 pos = aCamTransf.GetPos();
	glm::vec3 forward = aCamTransf.GetForward();
	glm::vec3 right = aCamTransf.GetRight();
	glm::vec3 up = aCamTransf.GetUp();
	if (Input::GetKey(Input::Keys::W))
	{
		pos += forward * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey(Input::Keys::S))
	{
		pos -= forward * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey(Input::Keys::D))
	{
		pos += right * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey(Input::Keys::A))
	{
		pos -= right * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey(Input::Keys::R))
	{
		pos += up * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey(Input::Keys::F))
	{
		pos -= up * myFlightSpeed * aDeltaTime;
	}
	aCamTransf.SetPos(pos);

	// General controls
	const float mouseWheelDelta = Input::GetMouseWheelDelta();
	if (Input::GetKeyPressed(Input::Keys::U) || mouseWheelDelta > 0.f)
	{
		myFlightSpeed *= 2;
	}
	if (Input::GetKeyPressed(Input::Keys::J) || mouseWheelDelta < 0.f)
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

	aCamTransf.SetRotation(yawRot * aCamTransf.GetRotation() * pitchRot);
}