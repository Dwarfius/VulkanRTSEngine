#include "Precomp.h"
#include "EditorMode.h"

#include "AnimationTest.h"
#include "IDRenderPass.h"
#include "NavMeshGen.h"

#include <Engine/Game.h>
#include <Engine/Input.h>
#include <Engine/Terrain.h>
#include <Engine/Light.h>
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
#include <Core/File.h>

#include <Graphics/Camera.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/GPUPipeline.h>

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>

EditorMode::EditorMode(Game& aGame)
	: myDefAssets(aGame)
	, myProto(aGame)
{
	AssetTracker& assetTracker = aGame.GetAssetTracker();
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

	PoolPtr<Light> light = aGame.GetLightSystem().AllocateLight();
	light.Get()->myColor = glm::vec3(1, 1, 1);
	light.Get()->myType = Light::Type::Directional;
	light.Get()->myAmbientIntensity = 1.f;
	myLights.push_back(std::move(light));
}

EditorMode::~EditorMode()
{
	if (myNavMesh)
	{
		delete myNavMesh;
	}
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

	UpdateTestSkeleton(aGame, aGame.IsPaused() ? 0 : aDeltaTime);
	if (myAnimTest)
	{
		myAnimTest->Update(aGame.IsPaused() ? 0 : aDeltaTime);
	}

	const float halfW = 100.f;
	const float halfH = 100.f;
	const float halfD = 100.f;

	DebugDrawer& debugDrawer = aGame.GetDebugDrawer();
	debugDrawer.AddLine(glm::vec3(-halfW, 0.f, 0.f), glm::vec3(halfW, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));
	debugDrawer.AddLine(glm::vec3(0.f, -halfH, 0.f), glm::vec3(0.f, halfH, 0.f), glm::vec3(0.f, 1.f, 0.f));
	debugDrawer.AddLine(glm::vec3(0.f, 0.f, -halfD), glm::vec3(0.f, 0.f, halfD), glm::vec3(0.f, 0.f, 1.f));

	solver.Update(aGame, aDeltaTime);
	myProto.Update(aGame, myDefAssets, aDeltaTime);

	DrawMenu(aGame);

	UpdatePickedObject(aGame);

	if (myNavMesh)
	{
		myNavMesh->DebugDraw(debugDrawer);
	}
}

void EditorMode::CreateBigWorld(Game& aGame)
{
	solver.Init(aGame);

	/*StaticString resPath = Resource::kAssetsFolder + "AnimTest/whale.gltf";
	StaticString resPath = Resource::kAssetsFolder + "AnimTest/riggedFigure.gltf";
	StaticString resPath = Resource::kAssetsFolder + "AnimTest/RiggedSimple.gltf";*/
	StaticString resPath = Resource::kAssetsFolder + "Boombox/BoomBox.gltf";
	//StaticString resPath = Resource::kAssetsFolder + "sparse.gltf";

	GLTFImporter gltfImporter;
	bool res = gltfImporter.Load(resPath.CStr());
	ASSERT(res);
	
	AssetTracker& assetTracker = aGame.GetAssetTracker();
	myGO = assetTracker.GetOrCreate<GameObject>("TestGameObject/testGO.go");

	Handle<Model> model = gltfImporter.GetModel(0);
	Handle<Texture> texture = gltfImporter.GetTexture(0);
	myGO->ExecLambdaOnLoad([&, model, texture](Resource* aRes) {
		myGO->GetComponent<VisualComponent>()->SetModel(model);
		myGO->GetComponent<VisualComponent>()->SetTexture(0, texture);
		Transform transf = myGO->GetWorldTransform();
		transf.SetScale({ 100, 100, 100 });
		myGO->SetWorldTransform(transf);

		GameObject* go = static_cast<GameObject*>(aRes);
		aGame.AddGameObject(go);
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
}

void EditorMode::CreateNavWorld(Game& aGame)
{
	Transform transf;
	transf.SetScale({ 30.f, 1.f, 30.f });
	CreateGOWithMesh(aGame, myDefAssets.GetPlane(), transf);

	// Single box to the side
	transf.SetScale({ 1, 1, 1 });
	transf.SetPos({ 0, 0.5f, 0 });
	CreateGOWithMesh(aGame, myDefAssets.GetBox(), transf);

	// ramps
	transf.SetScale({ 1, 0.1f, 1 });
	for (uint8_t i = 0; i <= 18; i++)
	{
		transf.SetRotation(glm::vec3{ glm::radians(i * 5.f), 0, 0 });
		transf.SetPos({ i * 1.5f - 1.5f * 9, 0, 2.f });
		CreateGOWithMesh(aGame, myDefAssets.GetBox(), transf);
	}

	// staircase
	transf.SetScale({ 1, 0.1f, 0.1f });
	for (uint8_t i = 0; i < 10; i++)
	{
		transf.SetPos({ 3.f, i * 0.1f + 0.05f, i * 0.1f });
		CreateGOWithMesh(aGame, myDefAssets.GetBox(), transf);
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

	if (ImGui::Begin("Editor"))
	{
		if (ImGui::BeginTabBar("EditorTable"))
		{
			if (ImGui::BeginTabItem("World Management"))
			{
				ImGui::Text("Current World");
				World& world = aGame.GetWorld();
				if (myWorldPath.empty())
				{
					myWorldPath = world.GetPath();
				}
				ImGui::InputText("World Path", myWorldPath);
				if (ImGui::Button("Save") && !myWorldPath.empty())
				{
					File::Delete(world.GetPath());
					aGame.GetAssetTracker().SaveAndTrack(myWorldPath, &world);
				}

				ImGui::Separator();

				ImGui::Text("Preset Worlds");
				if (ImGui::Button("Create Big World"))
				{
					CreateBigWorld(aGame);
				}
				if (ImGui::Button("Create Nav World"))
				{
					CreateNavWorld(aGame);
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Entity Creation"))
			{
				ImGui::Text("Shapes");
				Transform transf = aGame.GetCamera()->GetTransform();
				transf.Translate(transf.GetForward() * 5.f);
				transf.SetRotation(glm::vec3{ 0,0,0 });
				if (ImGui::Button("Create Plane"))
				{
					CreateGOWithMesh(aGame, myDefAssets.GetPlane(), transf);
				}
				if (ImGui::Button("Create Sphere"))
				{
					CreateGOWithMesh(aGame, myDefAssets.GetSphere(), transf);
				}
				if (ImGui::Button("Create Box"))
				{
					CreateGOWithMesh(aGame, myDefAssets.GetBox(), transf);
				}
				if (ImGui::Button("Create Cylinder"))
				{
					CreateGOWithMesh(aGame, myDefAssets.GetCylinder(), transf);
				}
				if (ImGui::Button("Create Cone"))
				{
					CreateGOWithMesh(aGame, myDefAssets.GetCone(), transf);
				}
				if (ImGui::Button("Create Mesh"))
				{
					CreateMesh(aGame, transf);
				}

				ImGui::Separator();

				if (ImGui::Button("Manage Lights"))
				{
					ManageLights(aGame);
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("NavMesh"))
			{
				ImGui::SliderAngle("Max Slope", &myMaxSlope, 0, 90);
				ImGui::Checkbox("Draw Generation AABB", &myDrawGenAABB);
				ImGui::Checkbox("Render Triangle Validity Checks", &myDebugTriangles);
				ImGui::Checkbox("Render Voxel Spans", &myRenderVoxels);

				if (ImGui::Button("Generate"))
				{
					if (myNavMesh)
					{
						delete myNavMesh;
					}
					myNavMesh = new NavMeshGen();
					NavMeshGen::Input input{ 
						&aGame.GetWorld(), 
						{-3.f,-5.f,-3.f}, 
						{3.f,5.f,3.f} 
					};
					NavMeshGen::Settings settings{ 
						glm::degrees(myMaxSlope),
						0,
						myDrawGenAABB,
						myDebugTriangles,
						myRenderVoxels
					};
					myNavMesh->Generate(input, settings, aGame);
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();

	if (myMenuFunction)
	{
		myMenuFunction(aGame);
	}
}

Handle<GameObject> EditorMode::CreateGOWithMesh(Game& aGame, Handle<Model> aModel, const Transform& aTransf)
{
	Handle<GameObject> go = new GameObject(aTransf);
	VisualComponent* visComp = go->AddComponent<VisualComponent>();
	visComp->SetModel(aModel);
	visComp->SetTextureCount(1);
	visComp->SetTexture(0, myDefAssets.GetUVTexture());
	visComp->SetPipeline(myDefAssets.GetPipeline());

	aGame.AddGameObject(go);

	return go;
}

void EditorMode::CreateMesh(Game& aGame, const Transform& aTransf)
{
	myMenuFunction = [&, this](Game& aGame) {
		myFileDialog.DrawFor<Model>();

		FileDialog::File selectedFile;
		if (myFileDialog.GetPickedFile(selectedFile))
		{
			AssetTracker& assetTracker = aGame.GetAssetTracker();
			Handle<Model> newModel = assetTracker.GetOrCreate<Model>(selectedFile.myPath);

			CreateGOWithMesh(aGame, newModel, aTransf);

			myMenuFunction = nullptr;
		}
	};
}

void EditorMode::ManageLights(Game& aGame)
{
	myMenuFunction = [this](Game& aGame) {
		bool isOpen = true;
		if (ImGui::Begin("Lights", &isOpen))
		{
			if (ImGui::Button("Add"))
			{
				myLights.emplace_back(aGame.GetLightSystem().AllocateLight());
			}
			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				myLights.clear();
			}

			if (ImGui::BeginListBox("Active"))
			{
				for (size_t i=0; i<myLights.size(); i++)
				{
					const PoolPtr<Light>& light = myLights[i];
					char buffer[8];
					Utils::StringFormat(buffer, "%u", i);
					if (ImGui::Selectable(buffer))
					{
						myPickedGO = nullptr;
						myPickedTerrain = nullptr;
						mySelectedLight = i;
					}
				}
				ImGui::EndListBox();
			}

			if (mySelectedLight != kInvalidInd)
			{
				Light& light = *myLights[mySelectedLight].Get();

				using IndType = Light::Type::UnderlyingType;
				IndType selectedInd = static_cast<IndType>(light.myType);
				if (ImGui::BeginCombo("Light Type", Light::Type::kNames[selectedInd]))
				{
					for (IndType i = 0; i < Light::Type::GetSize(); i++)
					{
						bool selected = selectedInd == i;
						if (ImGui::Selectable(Light::Type::kNames[i], &selected))
						{
							selectedInd = i;
							light.myType = static_cast<Light::Type>(selectedInd);
						}
					}

					ImGui::EndCombo();
				}
				
				glm::vec3 pos = light.myTransform.GetPos();
				if (ImGui::InputFloat3("Pos", glm::value_ptr(pos)))
				{
					light.myTransform.SetPos(pos);
				}
				aGame.GetDebugDrawer().AddSphere(pos, 0.1f, light.myColor);

				glm::vec3 eulerRot = light.myTransform.GetEuler();
				eulerRot = glm::degrees(eulerRot);
				if (ImGui::InputFloat3("Rot", glm::value_ptr(eulerRot)))
				{
					eulerRot = glm::radians(eulerRot);
					light.myTransform.SetRotation(eulerRot);
				}
				if (light.myType != Light::Type::Point)
				{
					aGame.GetDebugDrawer().AddLine(pos, pos + light.myTransform.GetForward(), light.myColor);
				}

				ImGui::ColorPicker3("Color", glm::value_ptr(light.myColor));
				ImGui::DragFloat3("Attenuation", glm::value_ptr(light.myAttenuation), 0.01f, 0, 2);
				ImGui::DragFloat("Ambient Intensity", &light.myAmbientIntensity, 0.001f, 0, 1);

				if (light.myType == Light::Type::Spot)
				{
					float innerLimit = glm::degrees(glm::acos(light.mySpotlightLimits[0]));
					ImGui::DragFloat("Inner Limit", &innerLimit, 0.1f, 0, 90);
					light.mySpotlightLimits[0] = glm::cos(glm::radians(innerLimit));
					float outerLimit = glm::degrees(glm::acos(light.mySpotlightLimits[1]));
					ImGui::DragFloat("Outer Limit", &outerLimit, 0.1f, 0, 90);
					outerLimit = glm::max(innerLimit, outerLimit);
					light.mySpotlightLimits[1] = glm::cos(glm::radians(outerLimit));
				}
			}
		}
		ImGui::End();

		if (!isOpen)
		{
			mySelectedLight = kInvalidInd;
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

	if (mySelectedLight != kInvalidInd)
	{
		Light& ligth = *myLights[mySelectedLight].Get();
		canUseInput &= !myGizmos.Draw(ligth.myTransform, aGame,
			Gizmos::Mode::Translation | Gizmos::Mode::Rotation);
	}

	if (myPickedGO)
	{
		const Renderable* renderable = myPickedGO->GetRenderable();
		const VisualObject& visObj = renderable->myVO;
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
			mySelectedLight = kInvalidInd;
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