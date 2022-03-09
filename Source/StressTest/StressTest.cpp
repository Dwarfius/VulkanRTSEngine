#include "Precomp.h"
#include "StressTest.h"

#include <Engine/Game.h>
#include <Engine/Terrain.h>
#include <Engine/Resources/OBJImporter.h>
#include <Engine/Components/VisualComponent.h>
#include <Engine/Input.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>

#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Camera.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Debug/DebugDrawer.h>

StressTest::StressTest(Game& aGame)
{
	std::random_device randDevice;
	myRandEngine = std::default_random_engine(randDevice());

	AssetTracker& assetTracker = aGame.GetAssetTracker();

	{
		constexpr uint32_t kTerrCells = 64;
		
		Terrain* terrain = new Terrain();
		terrain->Generate(glm::ivec2(kTerrCells, kTerrCells), 1, 1);

		terrain->PushHeightLevelColor(0.f, { 0, 0, 1 });
		terrain->PushHeightLevelColor(0.5f, { 0, 1, 1 });
		terrain->PushHeightLevelColor(1.f, { 1, 0, 1 });

		Handle<Pipeline> terrainPipeline = assetTracker.GetOrCreate<Pipeline>("TestTerrain/terrain.ppl");
		aGame.AddTerrain(terrain, terrainPipeline);
	}

	{
		OBJImporter importer;
		constexpr StaticString path = Resource::kAssetsFolder + "Tank/Tank.obj";
		[[maybe_unused]] bool loadRes = importer.Load(path);
		ASSERT_STR(loadRes, "Failed to load %s", path.CStr());
		myTankModel = importer.GetModel(0);
	}

	myTankTexture = assetTracker.GetOrCreate<Texture>("Tank/playerTank.img");
	myTankPipeline = assetTracker.GetOrCreate<Pipeline>("Engine/default.ppl");

	myTanks.reserve(1000);
}

void StressTest::Update(Game& aGame, float aDeltaTime)
{
	Profiler::ScopedMark mark("StressTest::Update");
	DrawUI(aGame);
	UpdateCamera(*aGame.GetCamera(), aDeltaTime);
	UpdateTanks(aGame, aDeltaTime);
}

void StressTest::DrawUI(Game& aGame)
{
	std::lock_guard imguiLock(aGame.GetImGUISystem().GetMutex());
	if (ImGui::Begin("Stress Test"))
	{
		ImGui::InputFloat("Tank Life", &myTankLife);
		ImGui::InputFloat("Spawn Rate", &mySpawnRate);
		ImGui::InputFloat("Spawn Square Side", &mySpawnSquareSide);
		ImGui::Text("Tanks alive: %llu", myTanks.size());
		const int32_t totalPop = static_cast<int32_t>(myTankLife * mySpawnRate);
		ImGui::Text("Target population: %d", totalPop);
	}
	ImGui::End();
}

void StressTest::UpdateTanks(Game& aGame, float aDeltaTime)
{
	myTankAccum += mySpawnRate * aDeltaTime;
	if (myTankAccum >= myTanks.size() + 1)
	{
		std::uniform_real_distribution<float> filter(
			-mySpawnSquareSide / 2.f,
			mySpawnSquareSide / 2.f
		);
		const uint32_t newCount = static_cast<uint32_t>(myTankAccum);
		const uint32_t toSpawnCount = static_cast<uint32_t>(newCount - myTanks.size());
		myTanks.resize(newCount);
		for (uint32_t i = 0; i < toSpawnCount; i++)
		{
			Tank& tank = myTanks[newCount - i - 1];
			tank.myLife = myTankLife;

			Transform tankTransf;
			tankTransf.SetPos({ filter(myRandEngine), 0, filter(myRandEngine) });
			tankTransf.SetScale({ 0.01f, 0.01f, 0.01f }); // the model is too large
			tank.myGO = new GameObject(tankTransf);
			VisualComponent* visualComp = tank.myGO->AddComponent<VisualComponent>();
			visualComp->SetModel(myTankModel);
			visualComp->SetTextureCount(1);
			visualComp->SetTexture(0, myTankTexture);
			visualComp->SetPipeline(myTankPipeline);
			aGame.AddGameObject(tank.myGO);
		}
	}

	for (Tank& tank : myTanks)
	{
		tank.myLife -= aDeltaTime;
		if (tank.myLife <= 0.f)
		{
			aGame.RemoveGameObject(tank.myGO);
			tank.myGO = Handle<GameObject>();
		}
	}

	const size_t removed = std::erase_if(myTanks, 
		[](const Tank& aTank) {
			return aTank.myLife <= 0.f;
		}
	);
	myTankAccum -= removed;
}

void StressTest::UpdateCamera(Camera& aCam, float aDeltaTime)
{
	constexpr float kSpeed = glm::radians(10.f);
	myRotationAngle += kSpeed * aDeltaTime;
	if (myRotationAngle > glm::two_pi<float>())
	{
		myRotationAngle -= glm::two_pi<float>();
	}

	constexpr glm::vec3 kTargetPos { 0, 0, 0 };
	const glm::vec3 initPos { mySpawnSquareSide, mySpawnSquareSide / 2, 0 };
	const glm::vec3 pos = Transform::RotateAround(
		initPos,
		{0, 0, 0},
		{ 0, myRotationAngle, 0 }
	);

	Transform& camTransf = aCam.GetTransform();
	camTransf.SetPos(pos);
	camTransf.LookAt(kTargetPos);
}