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

	{
		OBJImporter importer;
		constexpr StaticString path = Resource::kAssetsFolder + "sphere.obj";
		[[maybe_unused]] bool loadRes = importer.Load(path);
		ASSERT_STR(loadRes, "Failed to load %s", path.CStr());
		mySphereModel = importer.GetModel(0);
	}

	{
		myGreyTexture = new Texture();
		myGreyTexture->SetWidth(1);
		myGreyTexture->SetHeight(1);
		myGreyTexture->SetFormat(Texture::Format::UNorm_RGB);

		uint8_t* kGreyPixel = new uint8_t[]{ 64, 64, 64 };
		myGreyTexture->SetPixels(kGreyPixel);
	}

	myGreenTankTexture = assetTracker.GetOrCreate<Texture>("Tank/playerTank.img");
	myRedTankTexture = assetTracker.GetOrCreate<Texture>("Tank/enemyTank.img");
	myDefaultPipeline = assetTracker.GetOrCreate<Pipeline>("Engine/default.ppl");

	myTanks.reserve(1000);
}

void StressTest::Update(Game& aGame, float aDeltaTime)
{
	Profiler::ScopedMark mark("StressTest::Update");
	DrawUI(aGame);
	if (aGame.IsPaused())
	{
		return;
	}

	UpdateCamera(*aGame.GetCamera(), aDeltaTime);
	UpdateTanks(aGame, aDeltaTime);
	UpdateBalls(aGame, aDeltaTime);
}

void StressTest::DrawUI(Game& aGame)
{
	std::lock_guard imguiLock(aGame.GetImGUISystem().GetMutex());
	if (ImGui::Begin("Stress Test"))
	{
		ImGui::InputFloat("Spawn Rate", &mySpawnRate);
		ImGui::InputFloat("Spawn Square Side", &mySpawnSquareSide);
		ImGui::InputFloat("Tank Speed", &myTankSpeed);
		ImGui::Text("Tanks alive: %llu", myTanks.size());

		ImGui::Separator();

		ImGui::InputFloat("Shoot Cooldown", &myShootCD);
		ImGui::InputFloat("Shot Life", &myShotLife);
		ImGui::InputFloat("Shot Speed", &myShotSpeed);
		ImGui::Text("Cannonballs: %llu", myBalls.size());
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
			myTankSwitch = !myTankSwitch;

			Tank& tank = myTanks[newCount - i - 1];
			tank.myTeam = myTankSwitch;
			tank.myCooldown = myShootCD;

			const float xSpawn = mySpawnSquareSide / 2.f * (tank.myTeam ? 1 : -1);
			const float zSpawn = filter(myRandEngine);
			const float xTarget = -xSpawn;
			const float zTarget = filter(myRandEngine);
			tank.myDest = glm::vec3(xTarget, 0, zTarget);

			Transform tankTransf;
			tankTransf.SetPos({ xSpawn, 0, zSpawn });
			tankTransf.SetScale({ 0.01f, 0.01f, 0.01f }); // the model is too large
			tankTransf.LookAt(tank.myDest);
			tank.myGO = new GameObject(tankTransf);
			VisualComponent* visualComp = tank.myGO->AddComponent<VisualComponent>();
			visualComp->SetModel(myTankModel);
			visualComp->SetTextureCount(1);
			visualComp->SetTexture(0, tank.myTeam ? myGreenTankTexture : myRedTankTexture);
			visualComp->SetPipeline(myDefaultPipeline);
			aGame.AddGameObject(tank.myGO);
		}
	}

	for (Tank& tank : myTanks)
	{
		Transform transf = tank.myGO->GetWorldTransform();
		glm::vec3 dist = tank.myDest - transf.GetPos();
		dist.y = 0;
		const float sqrlength = glm::length2(dist);

		if (sqrlength <= 1.f)
		{
			aGame.RemoveGameObject(tank.myGO);
			tank.myGO = Handle<GameObject>();
			continue;
		}

		transf.Translate(transf.GetForward() * myTankSpeed * aDeltaTime);
		tank.myGO->SetWorldTransform(transf);

		tank.myCooldown -= aDeltaTime;
		if (tank.myCooldown < 0)
		{
			tank.myCooldown += myShootCD;

			Transform ballTransf;
			ballTransf.SetPos(transf.GetPos()
				+ transf.GetForward() * 0.2f
				+ transf.GetUp() * 0.85f
			);
			ballTransf.SetScale({ 0.2f, 0.2f, 0.2f });

			Ball ball;
			ball.myLife = myShotLife;

			glm::vec3 initVelocity = transf.GetForward();
			initVelocity.y += 0.5f;
			ball.myVel = glm::normalize(initVelocity) * myShotSpeed;

			ball.myGO = new GameObject(ballTransf);
			VisualComponent* visualComp = ball.myGO->AddComponent<VisualComponent>();
			visualComp->SetModel(mySphereModel);
			visualComp->SetTextureCount(1);
			visualComp->SetTexture(0, myGreyTexture);
			visualComp->SetPipeline(myDefaultPipeline);
			aGame.AddGameObject(ball.myGO);

			myBalls.push_back(ball);
		}
	}

	const size_t removed = std::erase_if(myTanks, 
		[](const Tank& aTank) {
			return !aTank.myGO.IsValid();
		}
	);
	myTankAccum -= removed;
}

void StressTest::UpdateBalls(Game& aGame, float aDeltaTime)
{
	constexpr static float kGravity = 9.8f;
	for (Ball& ball : myBalls)
	{
		ball.myLife -= aDeltaTime;
		if (ball.myLife < 0)
		{
			aGame.RemoveGameObject(ball.myGO);
			ball.myGO = Handle<GameObject>();
			continue;
		}

		
		ball.myVel.y -= kGravity * aDeltaTime;

		Transform transf = ball.myGO->GetWorldTransform();
		transf.Translate(ball.myVel * aDeltaTime);
		ball.myGO->SetWorldTransform(transf);
	}

	std::erase_if(myBalls,
		[](const Ball& aBall) {
			return !aBall.myGO.IsValid();
		}
	);
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