#include "Precomp.h"
#include "EditorMode.h"

#include "AnimationTest.h"
#include "IDRenderPass.h"

#include <Engine/Game.h>
#include <Engine/Input.h>
#include <Engine/Terrain.h>
#include <Engine/Components/PhysicsComponent.h>
#include <Engine/Components/VisualComponent.h>
#include <Engine/GameObject.h>
#include <Engine/VisualObject.h>
#include <Engine/Animation/AnimationController.h>
#include <Engine/Animation/AnimationSystem.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>
#include <Engine/Resources/GLTFImporter.h>
#include <Engine/Resources/ImGUISerializer.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Utils.h>

#include <Graphics/Camera.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/GPUPipeline.h>

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsShapes.h>

EditorMode::EditorMode(Game& aGame)
	: myDefAssets(aGame)
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
	AssetTracker& assetTracker = aGame.GetAssetTracker();
	assetTracker.SaveAndTrack("TestGameObject/modelTestSave.model", model);

	myGO = assetTracker.GetOrCreate<GameObject>("TestGameObject/testGO.go");
	myGO->ExecLambdaOnLoad([&](Resource* aRes){
		GameObject* go = static_cast<GameObject*>(aRes);
		aGame.AddGameObject(go);

		aGame.GetAssetTracker().SaveAndTrack("TestGameObject/goSaveTest.go", go);
	});

	{
		constexpr float kTerrSize = 18000; // meters
		constexpr float kResolution = 928; // pixels
		Terrain* terrain = new Terrain();
		// Heightmaps generated via https://tangrams.github.io/heightmapper/
		Handle<Texture> terrainText = aGame.GetAssetTracker().GetOrCreate<Texture>("TestTerrain/Tynemouth-tangrams.img");
		terrain->Load(terrainText, kTerrSize / kResolution, 1000.f);

		terrain->PushHeightLevelColor(10.f, { 0, 0, 1 });
		terrain->PushHeightLevelColor(15.f, { 1, 0.8f, 0 });
		terrain->PushHeightLevelColor(25.f, { 0, 1, 0.15f });
		terrain->PushHeightLevelColor(50.f, { 0.5f, 0.5f, 0.5f });
		terrain->PushHeightLevelColor(100.f, { 1, 1, 1 });

		//constexpr uint32_t kTerrCells = 64;
		//terr->Generate(glm::ivec2(kTerrCells, kTerrCells), 1, 10);

		Handle<Pipeline> terrainPipeline = aGame.GetAssetTracker().GetOrCreate<Pipeline>("TestTerrain/terrain.ppl");
		aGame.AddTerrain(terrain, terrainPipeline);
	}

	myAnimTest = new AnimationTest(aGame);

	solver.Init(aGame);

	Handle<Pipeline> idDefaultPipeline = assetTracker.GetOrCreate<Pipeline>(
		"Editor/IDPipeline.ppl"
	);
	Handle<Pipeline> idSkinnedPipeline = assetTracker.GetOrCreate<Pipeline>(
		"Editor/IDSkinnedPipeline.ppl"
	);
	Handle<Pipeline> idTerrainPipeline = assetTracker.GetOrCreate<Pipeline>(
		"Editor/IDTerrainPipeline.ppl"
	);
	Graphics& graphics = *aGame.GetGraphics();
	myIDRenderPass = new IDRenderPass(
		graphics,
		graphics.GetOrCreate(idDefaultPipeline).Get<GPUPipeline>(),
		graphics.GetOrCreate(idSkinnedPipeline).Get<GPUPipeline>(),
		graphics.GetOrCreate(idTerrainPipeline).Get<GPUPipeline>()
	);
	aGame.GetGraphics()->AddRenderPass(myIDRenderPass);
	aGame.AddRenderGameObjectCallback(
		[idRenderPass = myIDRenderPass]
		(Graphics& aGraphics, Renderable& aRenderable, Camera& aCamera)
	{
		idRenderPass->ScheduleRenderable(aGraphics, aRenderable, aCamera);
	});
	aGame.AddRenderTerrainCallback(
		[idRenderPass = myIDRenderPass](Graphics& aGraphics, Terrain& aTerrain,
			VisualObject& aVisObject, Camera& aCamera)
	{
		idRenderPass->ScheduleTerrain(aGraphics, aTerrain, aVisObject, aCamera);
	});
}

EditorMode::~EditorMode()
{
	delete myAnimTest;
}

void EditorMode::Update(Game& aGame, float aDeltaTime, PhysicsWorld* aWorld)
{
	Camera* cam = aGame.GetCamera();
	Transform& camTransf = cam->GetTransform();

	if (Input::GetMouseBtn(1))
	{
		HandleCamera(camTransf, aDeltaTime);
	}

	if (aWorld && Input::GetMouseBtnPressed(0))
	{
		glm::vec3 from = camTransf.GetPos();
		glm::vec3 dir = camTransf.GetForward();

		PhysicsEntity* physEntity;
		if (aWorld->RaycastClosest(from, dir, 100.f, physEntity)
			&& physEntity->GetType() == PhysicsEntity::Type::Dynamic)
		{
			physEntity->AddForce(dir * 100.f);
		}
	}

	if (Input::GetMouseBtnPressed(2))
	{
		AssetTracker& assetTracker = aGame.GetAssetTracker();
		AnimationSystem& animSystem = aGame.GetAnimationSystem();

		Transform objTransf = myGLTFImporter.GetTransform(0);
		objTransf.SetPos(objTransf.GetPos() + camTransf.GetPos());
		Handle<GameObject> newGO = new GameObject(objTransf);
		
		GameObject* go = newGO.Get();
		go->CreateRenderable();
		VisualObject& vo = go->GetRenderable().Get()->myVO;

		Handle<Model> model = myGLTFImporter.GetModel(0);

		vo.SetModel(model);
		if (myGLTFImporter.GetTextureCount() > 0)
		{
			vo.SetTexture(myGLTFImporter.GetTexture(0));
		}
		else
		{
			vo.SetTexture(assetTracker.GetOrCreate<Texture>("gray.img"));
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
			vo.SetPipeline(assetTracker.GetOrCreate<Pipeline>("AnimTest/skinned.ppl"));
		}
		else
		{
			vo.SetPipeline(assetTracker.GetOrCreate<Pipeline>("Engine/default.ppl"));
		}

		myGOs.push_back(go);

		aGame.AddGameObject(newGO);
	}

	UpdateTestSkeleton(aGame, aGame.IsPaused() ? 0 : aDeltaTime);
	myAnimTest->Update(aGame.IsPaused() ? 0 : aDeltaTime);

	const Terrain* terrain = aGame.GetTerrain(0);
	const float halfW = terrain->GetWidth() / 2.f;
	const float halfH = 2;
	const float halfD = terrain->GetDepth() / 2.f;

	DebugDrawer& debugDrawer = aGame.GetDebugDrawer();
	debugDrawer.AddLine(glm::vec3(-halfW, 0.f, 0.f), glm::vec3(halfW, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));
	debugDrawer.AddLine(glm::vec3(0.f, -halfH, 0.f), glm::vec3(0.f, halfH, 0.f), glm::vec3(0.f, 1.f, 0.f));
	debugDrawer.AddLine(glm::vec3(0.f, 0.f, -halfD), glm::vec3(0.f, 0.f, halfD), glm::vec3(0.f, 0.f, 1.f));

	solver.Update(aGame, aDeltaTime);

	DrawMenu(aGame);

	UpdatePickedObject(aGame);
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

	glm::vec3 pitchDelta(mouseDelta.y, 0.f, 0.f);
	pitchDelta *= myMouseSensitivity;
	const glm::quat pitchRot(glm::radians(pitchDelta));

	glm::vec3 yawDelta(0.f, -mouseDelta.x, 0.f);
	yawDelta *= myMouseSensitivity;
	const glm::quat yawRot(glm::radians(yawDelta));

	aCamTransf.SetRotation(yawRot * aCamTransf.GetRotation() * pitchRot);
}

void EditorMode::DrawMenu(Game& aGame)
{
	std::lock_guard lock(aGame.GetImGUISystem().GetMutex());
	if (ImGui::Begin("Entity Creation"))
	{
		ImGui::Text("Shapes");
		if (ImGui::Button("Create Plane"))
		{
			CreateGOWithMesh(aGame, myDefAssets.GetPlane());
		}
		if (ImGui::Button("Create Sphere"))
		{
			CreateGOWithMesh(aGame, myDefAssets.GetSphere());
		}
		if (ImGui::Button("Create Box"))
		{
			CreateGOWithMesh(aGame, myDefAssets.GetBox());
		}
		if (ImGui::Button("Create Cylinder"))
		{
			CreateGOWithMesh(aGame, myDefAssets.GetCylinder());
		}
		if (ImGui::Button("Create Cone"))
		{
			CreateGOWithMesh(aGame, myDefAssets.GetCone());
		}
		if (ImGui::Button("Create Mesh"))
		{
			CreateMesh(aGame);
		}
	}
	ImGui::End();

	if (myMenuFunction)
	{
		myMenuFunction(aGame);
	}
}

void EditorMode::CreateGOWithMesh(Game& aGame, Handle<Model> aModel)
{
	Transform transf = aGame.GetCamera()->GetTransform();
	transf.Translate(transf.GetForward() * 5.f);
	transf.SetRotation(glm::vec3{ 0,0,0 });
	Handle<GameObject> go = new GameObject(transf);
	VisualComponent* visComp = go->AddComponent<VisualComponent>();
	visComp->SetModel(aModel);
	visComp->SetTextureCount(1);
	visComp->SetTexture(0, myDefAssets.GetUVTexture());
	visComp->SetPipeline(myDefAssets.GetPipeline());

	aGame.AddGameObject(go);
}

void EditorMode::CreateMesh(Game& aGame)
{
	myMenuFunction = [this](Game& aGame) {
		myFileDialog.DrawFor<Model>();

		FileDialog::File selectedFile;
		if (myFileDialog.GetPickedFile(selectedFile))
		{
			AssetTracker& assetTracker = aGame.GetAssetTracker();
			Handle<Model> newModel = assetTracker.GetOrCreate<Model>(selectedFile.myPath);

			CreateGOWithMesh(aGame, newModel);

			myMenuFunction = nullptr;
		}
	};
}

void EditorMode::UpdatePickedObject(Game& aGame)
{
	if(myPickedGO || myPickedTerrain)
	{
		std::lock_guard lock(aGame.GetImGUISystem().GetMutex());
		if (ImGui::Begin("Picked Entity"))
		{
			if (myPickedGO)
			{
				char buffer[33];
				myPickedGO->GetUID().GetString(buffer);
				ImGui::Text("Picked Game Object: %s", buffer);

				ImGUISerializer serializer(aGame.GetAssetTracker());
				myPickedGO->Serialize(serializer);
			}
			else if (myPickedTerrain)
			{
				ImGui::Text("Picked terrain");
			}
		}
		ImGui::End();
	}

	bool canUseInput = !ImGui::GetIO().WantCaptureMouse;
	if (myPickedGO)
	{
		const PoolPtr<Renderable>& renderable = myPickedGO->GetRenderable();
		const VisualObject& visObj = renderable.Get()->myVO;
		const glm::vec3 center = visObj.GetCenter();
		const float radius = visObj.GetRadius();
		aGame.GetDebugDrawer().AddSphere(
			center,
			radius,
			{ 0, 1, 0 }
		);

		canUseInput &= !myGizmos.Draw(*myPickedGO, aGame);
	}

	if(canUseInput && Input::GetMouseBtnPressed(0))
	{
		glm::uvec2 mousePos;
		mousePos.x = static_cast<uint32_t>(Input::GetMousePos().x);
		mousePos.y = static_cast<uint32_t>(Input::GetMousePos().y);
		myIDRenderPass->GetPickedEntity(mousePos,
			[&](IDRenderPass::PickedObject& anObj)
		{
			myPickedGO = nullptr;
			myPickedTerrain = nullptr;
			std::visit([&](auto&& aValue) {
				using T = std::decay_t<decltype(aValue)>;
				if constexpr (std::is_same_v<T, GameObject*>)
				{
					myPickedGO = aValue;
				}
				else if constexpr (std::is_same_v<T, Terrain*>)
				{
					myPickedTerrain = aValue;
				}
				else
				{
					static_assert(std::is_same_v<T, std::monostate>,
						"Not all cases handled!");
				}
			}, anObj);
		});
	}
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