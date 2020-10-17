#include "Precomp.h"
#include "EditorMode.h"

#include "../Game.h"
#include "../Input.h"
#include "PhysicsComponent.h"
#include "../GameObject.h"
#include "../VisualObject.h"
#include "../Animation/AnimationController.h"

#include <Graphics/Camera.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsShapes.h>

EditorMode::EditorMode(PhysicsWorld& aWorld)
	: myMouseSensitivity(0.1f)
	, myFlightSpeed(2.f)
	, myDemoWindowVisible(false)
	, mySelectedBone(Skeleton::kInvalidIndex)
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

	if (!myTestSkeleton.IsValid())
	{
		InitTestSkeleton(aGame.GetAnimationSystem());
	}

	UpdateTestSkeleton(aGame, aGame.IsPaused() ? 0 : aDeltaTime);

	myProfilerUI.Draw();
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

void EditorMode::InitTestSkeleton(AnimationSystem& anAnimSystem)
{
	myTestSkeleton = anAnimSystem.AllocateSkeleton(4);
	Skeleton* skeleton = myTestSkeleton.Get();

	skeleton->AddBone(Skeleton::kInvalidIndex, {});
	skeleton->AddBone(0, { glm::vec3(0, 1, 0), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->AddBone(1, { glm::vec3(0, 0, 1), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->AddBone(2, { glm::vec3(0, 1, 0), glm::vec3(0.f), glm::vec3(1.f) });

	// 4 second animation that forces a diamond movement
	myTestClip = std::make_unique<AnimationClip>(4, true);
	std::vector<AnimationClip::Mark> yTrack{ {0, -0.5f}, {2, 0.5f}, {4, -0.5f} };
	std::vector<AnimationClip::Mark> xTrack{ {0, 0}, {1, -0.5f}, {3, 0.5f}, {4, 0.f} };
	std::vector<AnimationClip::Mark> zRotTrack{ {0, 0}, {4, 2 * glm::pi<float>()} };

	myTestClip->AddTrack(1, AnimationClip::Property::PosX, AnimationClip::Interpolation::Linear, xTrack);
	myTestClip->AddTrack(1, AnimationClip::Property::PosY, AnimationClip::Interpolation::Linear, yTrack);
	myTestClip->AddTrack(1, AnimationClip::Property::RotZ, AnimationClip::Interpolation::Linear, zRotTrack);

	myAnimController = anAnimSystem.AllocateController(myTestSkeleton);
	myAnimController.Get()->PlayClip(myTestClip.get());
}

void EditorMode::UpdateTestSkeleton(Game& aGame, float aDeltaTime)
{
	{
		tbb::mutex::scoped_lock lock(aGame.GetImGUISystem().GetMutex());
		if (ImGui::Begin("Skeleton Settings"))
		{
			DrawBoneHierarchy();
			DrawBoneInfo();
		}
		ImGui::End();
	}

	myTestSkeleton.Get()->DebugDraw(aGame.GetDebugDrawer(), { glm::vec3(1), glm::vec3(0), glm::vec3(1) });
}

void EditorMode::DrawBoneHierarchy()
{
	Skeleton* skeleton = myTestSkeleton.Get();
	if (ImGui::TreeNode("Skeleton"))
	{
		char formatBuffer[128];
		for (Skeleton::BoneIndex index = 0;
			index < skeleton->GetBoneCount();
			index++)
		{
			glm::vec3 worldPos = skeleton->GetBoneWorldTransform(index).GetPos();
			sprintf(formatBuffer, "%u", index);

			const bool isSelectedBone = index == mySelectedBone;

			if (isSelectedBone)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 255));
			}

			if (ImGui::Button(formatBuffer))
			{
				mySelectedBone = index;
			}
			ImGui::SameLine();
			ImGui::Text("parent:(%u) - pos:(%f %f %f)",
				skeleton->GetParentIndex(index),
				worldPos.x, worldPos.y, worldPos.z);

			if (isSelectedBone)
			{
				ImGui::PopStyleColor();
			}
		}

		if (ImGui::Button("Clear Selection"))
		{
			mySelectedBone = Skeleton::kInvalidIndex;
		}

		const AnimationController* controller = myAnimController.Get();
		ImGui::Text("Animation time: %f", controller->GetTime());

		ImGui::TreePop();
	}
}

void EditorMode::DrawBoneInfo()
{
	if (mySelectedBone == Skeleton::kInvalidIndex)
	{
		return;
	}

	Skeleton* skeleton = myTestSkeleton.Get();
	if (ImGui::TreeNode("Bone Info"))
	{
		if (ImGui::TreeNode("World Transform"))
		{
			Transform transform = skeleton->GetBoneWorldTransform(mySelectedBone);
			glm::vec3 pos = transform.GetPos();
			const bool posChanged = ImGui::InputFloat3("Pos", glm::value_ptr(pos));
			if (posChanged)
			{
				transform.SetPos(pos);
			}

			glm::vec3 rot = transform.GetEuler();
			const bool rotChanged = ImGui::InputFloat3("Rot", glm::value_ptr(rot));
			if (rotChanged)
			{
				transform.SetRotation(rot);
			}

			glm::vec3 scale = transform.GetScale();
			const bool scaleChanged = ImGui::InputFloat3("Scale", glm::value_ptr(scale));
			if (scaleChanged)
			{
				transform.SetScale(scale);
			}

			if (posChanged || rotChanged || scaleChanged)
			{
				skeleton->SetBoneWorldTransform(mySelectedBone, transform);
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Local Transform"))
		{
			Transform transform = skeleton->GetBoneLocalTransform(mySelectedBone);
			glm::vec3 pos = transform.GetPos();
			const bool posChanged = ImGui::InputFloat3("Pos", glm::value_ptr(pos));
			if (posChanged)
			{
				transform.SetPos(pos);
			}

			glm::vec3 rot = transform.GetEuler();
			const bool rotChanged = ImGui::InputFloat3("Rot", glm::value_ptr(rot));
			if (rotChanged)
			{
				transform.SetRotation(rot);
			}

			glm::vec3 scale = transform.GetScale();
			const bool scaleChanged = ImGui::InputFloat3("Scale", glm::value_ptr(scale));
			if (scaleChanged)
			{
				transform.SetScale(scale);
			}

			if (posChanged || rotChanged || scaleChanged)
			{
				skeleton->SetBoneLocalTransform(mySelectedBone, transform);
			}
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
}