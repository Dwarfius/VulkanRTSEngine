#include "Precomp.h"
#include "EditorMode.h"

#include "Game.h"
#include "Input.h"
#include "Components/PhysicsComponent.h"
#include "Components/VisualComponent.h"
#include "GameObject.h"
#include "VisualObject.h"
#include "Animation/AnimationController.h"
#include "Animation/AnimationSystem.h"
#include "Systems/ImGUI/ImGUISystem.h"
#include "Resources/GLTFImporter.h"

#include <Core/Resources/AssetTracker.h>
#include <Core/Utils.h>

#include <Graphics/Camera.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsShapes.h>

EditorMode::EditorMode(Game& aGame)
{
	myPhysShape = std::make_shared<PhysicsShapeBox>(glm::vec3(0.5f));

	/*StaticString resPath = Resource::kAssetsFolder + "AnimTest/whale.gltf";
	StaticString resPath = Resource::kAssetsFolder + "AnimTest/riggedFigure.gltf";
	StaticString resPath = Resource::kAssetsFolder + "AnimTest/RiggedSimple.gltf";
	StaticString resPath = Resource::kAssetsFolder + "Boombox/BoomBox.gltf";*/
	StaticString resPath = Resource::kAssetsFolder + "sparse.gltf";
	bool res = myGLTFImporter.Load(resPath.CStr());
	ASSERT(res);
	Handle<Model> model = myGLTFImporter.GetModel(0);
	aGame.GetAssetTracker().SaveAndTrack("TestGameObject/modelTestSave.model", model);

	myGO = aGame.GetAssetTracker().GetOrCreate<GameObject>("TestGameObject/testGO.go");
	myGO->ExecLambdaOnLoad([&](Resource* aRes){
		GameObject* go = static_cast<GameObject*>(aRes);
		aGame.AddGameObject(go);

		aGame.GetAssetTracker().SaveAndTrack("TestGameObject/goSaveTest.go", go);
	});

	myTopBar.Register("Widgets/Demo", [&] { myDemoWindowVisible = true; });
	myTopBar.Register("Widgets/Camera Info", [&] { myShowCameraInfo = true; });
	myTopBar.Register("Widgets/Profiler", [&] { myShowProfiler = true; });
	myTopBar.Register("Widgets/Entities View", [&] { myShowEntitiesView = true; });
	myTopBar.Register("File/Import OBJ", [&] { myShowObjImport = true; });
	myTopBar.Register("File/Import Gltf", [&] { myShowGltfImport = true; });
	myTopBar.Register("File/Import Texture", [&] { myShowTextureImport = true; });
}

void EditorMode::Update(Game& aGame, float aDeltaTime, PhysicsWorld& aWorld)
{
	Camera* cam = aGame.GetCamera();
	Transform& camTransf = cam->GetTransform();

	if (Input::GetMouseBtn(1))
	{
		HandleCamera(camTransf, aDeltaTime);
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
		AssetTracker& assetTracker = aGame.GetAssetTracker();
		AnimationSystem& animSystem = aGame.GetAnimationSystem();

		Transform objTransf = myGLTFImporter.GetTransform(0);
		objTransf.SetPos(objTransf.GetPos() +  camTransf.GetPos());
		Handle<GameObject> newGO = new GameObject(objTransf);
		
		GameObject* go = newGO.Get();
		VisualObject* vo = new VisualObject(*go);
		go->SetVisualObject(vo);

		Handle<Model> model = myGLTFImporter.GetModel(0);

		vo->SetModel(model);
		if (myGLTFImporter.GetTextureCount() > 0)
		{
			vo->SetTexture(myGLTFImporter.GetTexture(0));
		}
		else
		{
			vo->SetTexture(assetTracker.GetOrCreate<Texture>("gray.img"));
		}

		if(myGLTFImporter.GetSkeletonCount() > 0)
		{
			PoolPtr<Skeleton> testSkeleton = animSystem.AllocateSkeleton(0);
			*testSkeleton.Get() = myGLTFImporter.GetSkeleton(0);
			go->SetSkeleton(std::move(testSkeleton));

			if (myGLTFImporter.GetClipCount() > 0)
			{
				PoolPtr<AnimationController> animController = animSystem.AllocateController(go->GetSkeleton());
				animController.Get()->PlayClip(myGLTFImporter.GetAnimClip(0).Get());
				go->SetAnimController(std::move(animController));
			}
			vo->SetPipeline(assetTracker.GetOrCreate<Pipeline>("AnimTest/skinned.ppl"));
		}
		else
		{
			vo->SetPipeline(assetTracker.GetOrCreate<Pipeline>("Engine/default.ppl"));
		}

		myGOs.push_back(go);

		aGame.AddGameObject(newGO);
	}

	UpdateTestSkeleton(aGame, aGame.IsPaused() ? 0 : aDeltaTime);

	myTopBar.Draw();

	{
		std::lock_guard lock(aGame.GetImGUISystem().GetMutex());
		if (myDemoWindowVisible)
		{
			ImGui::ShowDemoWindow(&myDemoWindowVisible);
		}

		if (myShowCameraInfo)
		{
			if (ImGui::Begin("Camera", &myShowCameraInfo))
			{
				ImGui::Text("Pos: %f %f %f", camTransf.GetPos().x, camTransf.GetPos().y, camTransf.GetPos().z);
			}
			ImGui::End();
		}
	}

	if (myShowProfiler)
	{
		myProfilerUI.Draw(myShowProfiler);
	}

	if (myShowEntitiesView)
	{
		myEntitiesView.DrawDialog(aGame, myShowEntitiesView);
	}

	if (myShowObjImport)
	{
		myObjImport.Draw(myShowObjImport);
	}

	if (myShowGltfImport)
	{
		myGltfImport.Draw(myShowGltfImport);
	}

	if (myShowTextureImport)
	{
		myTextureImport.Draw(myShowTextureImport);
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

void EditorMode::AddTestSkeleton(Game& aGame)
{
	AnimationSystem& animSystem = aGame.GetAnimationSystem();
	PoolPtr<Skeleton> testSkeleton = animSystem.AllocateSkeleton(4);
	Skeleton* skeleton = testSkeleton.Get();

	skeleton->AddBone(Skeleton::kInvalidIndex, {});
	skeleton->AddBone(0, { glm::vec3(0, 1, 0), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->AddBone(1, { glm::vec3(0, 0, 1), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->AddBone(2, { glm::vec3(0, 1, 0), glm::vec3(0.f), glm::vec3(1.f) });

	PoolPtr<AnimationController> animController = animSystem.AllocateController(testSkeleton);

	Camera& cam = *aGame.GetCamera();
	Transform objTransf;
	objTransf.SetPos(cam.GetTransform().GetPos());
	Handle<GameObject> newGO = new GameObject(objTransf);
	aGame.AddGameObject(newGO);
	GameObject* go = newGO.Get();
	go->SetSkeleton(std::move(testSkeleton));
	go->SetAnimController(std::move(animController));
	myGOs.push_back(go);
}

void EditorMode::UpdateTestSkeleton(Game& aGame, float aDeltaTime)
{
	if (Input::GetKeyPressed(Input::Keys::F3))
	{
		myShowSkeletonUI = !myShowSkeletonUI;
	}

	if(myShowSkeletonUI)
	{
		std::lock_guard lock(aGame.GetImGUISystem().GetMutex());
		if (ImGui::Begin("Skeleton Settings"))
		{
			ImGui::InputInt("Skeleton Add Count", &myAddSkeletonCount);
			if (ImGui::Button("Add"))
			{
				for (int i = 0; i < myAddSkeletonCount; i++)
				{
					AddTestSkeleton(aGame);
				}
			}

			ImGui::InputInt("Skeleton Index", &mySelectedSkeleton);

			DrawBoneHierarchy(mySelectedSkeleton);
			DrawBoneInfo(mySelectedSkeleton);
		}
		ImGui::End();
	}

	for (const GameObject* gameObject : myGOs)
	{
		const PoolPtr<Skeleton>& skeletonPtr = gameObject->GetSkeleton();
		if (skeletonPtr.IsValid())
		{
			skeletonPtr.Get()->DebugDraw(aGame.GetDebugDrawer(), gameObject->GetWorldTransform());
		}
	}
}

void EditorMode::DrawBoneHierarchy(int aSkeletonIndex)
{
	if (aSkeletonIndex >= 0 
		&& aSkeletonIndex < myGOs.size() 
		&& ImGui::TreeNode("SkeletonTreeNode", "Skeleton %d", aSkeletonIndex))
	{
		const PoolPtr<Skeleton>& skeletonPtr = myGOs[aSkeletonIndex]->GetSkeleton();
		if (!skeletonPtr.IsValid())
		{
			ImGui::TreePop();
			return;
		}

		const Skeleton* skeleton = skeletonPtr.Get();

		char formatBuffer[128];
		for (Skeleton::BoneIndex index = 0;
			index < skeleton->GetBoneCount();
			index++)
		{
			glm::vec3 worldPos = skeleton->GetBoneWorldTransform(index).GetPos();
			Utils::StringFormat(formatBuffer, "%u", index);

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

		if (ImGui::Button("Clear Selected Bone"))
		{
			mySelectedBone = Skeleton::kInvalidIndex;
		}

		const PoolPtr<AnimationController>& controllerPtr = myGOs[aSkeletonIndex]->GetAnimController();
		if (!controllerPtr.IsValid())
		{
			ImGui::TreePop();
			return;
		}

		const AnimationController* controller = controllerPtr.Get();
		ImGui::Text("Animation time: %f", controller->GetTime());

		ImGui::TreePop();
	}
}

void EditorMode::DrawBoneInfo(int aSkeletonIndex)
{
	if (mySelectedBone == Skeleton::kInvalidIndex
		|| aSkeletonIndex == -1
		|| aSkeletonIndex >= myGOs.size())
	{
		return;
	}

	if (ImGui::TreeNode("Bone Info"))
	{
		PoolWeakPtr<Skeleton> skeletonPtr = myGOs[aSkeletonIndex]->GetSkeleton();
		if (!skeletonPtr.IsValid())
		{
			ImGui::TreePop();
			return;
		}

		Skeleton* skeleton = skeletonPtr.Get();
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