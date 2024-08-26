#include "Precomp.h"
#include "StressTest.h"

#include <Engine/Game.h>
#include <Engine/Terrain.h>
#include <Engine/Resources/OBJImporter.h>
#include <Engine/Components/PhysicsComponent.h>
#include <Engine/Components/VisualComponent.h>
#include <Engine/Input.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>
#include <Engine/Light.h>

#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Camera.h>

#include <Physics/PhysicsShapes.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Debug/DebugDrawer.h>
#include <Core/Shapes.h>

StressTest::StressTest(Game& aGame)
	: myGrid({ -200, -200 }, 400, 50) // TODO: add support for resizing
{
	std::random_device randDevice;
	myRandEngine = std::default_random_engine(randDevice());

	AssetTracker& assetTracker = aGame.GetAssetTracker();

	CreateTerrain(aGame, mySpawnSquareSide);

	{
		OBJImporter importer;
		constexpr StaticString path = Resource::kAssetsFolder + "Tank/Tank.obj";
		[[maybe_unused]] bool loadRes = importer.Load(path);
		ASSERT_STR(loadRes, "Failed to load {}", path);
		myTankModel = importer.GetModel(0);

		const glm::vec3 aabbMin = myTankModel->GetAABBMin();
		const glm::vec3 aabbMax = myTankModel->GetAABBMax();
		const glm::vec3 halfExtents = (aabbMax - aabbMin) / 2.f * kTankScale;
		myTankShape = std::make_shared<PhysicsShapeBox>(
			halfExtents
		);
	}

	{
		OBJImporter importer;
		constexpr StaticString path = Resource::kAssetsFolder + "sphere.obj";
		[[maybe_unused]] bool loadRes = importer.Load(path);
		ASSERT_STR(loadRes, "Failed to load {}", path);
		mySphereModel = importer.GetModel(0);

		const glm::vec3 aabbMin = mySphereModel->GetAABBMin();
		const glm::vec3 aabbMax = mySphereModel->GetAABBMax();
		const glm::vec3 halfExtents = (aabbMax - aabbMin) / 2.f * kBallScale;
		myBallShape = std::make_shared<PhysicsShapeSphere>(
			halfExtents.x
		);
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

	myLight = aGame.GetLightSystem().AllocateLight();
	Light& light = *myLight.Get();
	light.myType = Light::Type::Directional;
	light.myColor = { 1, 1, 1 };
	light.myAmbientIntensity = 0.9f;
	light.myTransform.LookAt({ 0, -10, 0 });
}

void StressTest::Update(Game& aGame, float aDeltaTime)
{
	Profiler::ScopedMark mark("StressTest::Update");
	DrawUI(aGame, aDeltaTime);
	if (aGame.IsPaused())
	{
		return;
	}

	if (myWipeEverything)
	{
		WipeEverything(aGame);
		myWipeEverything = false;

		uint32_t nextPOT = 1;
		while (mySpawnSquareSide > nextPOT)
		{
			nextPOT <<= 1;
		}
		if (aGame.GetTerrain(0)->GetWidth() != nextPOT)
		{
			aGame.RemoveTerrain(0);
			CreateTerrain(aGame, nextPOT);
		}
	}

	UpdateCamera(*aGame.GetCamera(), aDeltaTime);
	UpdateTanks(aGame, aDeltaTime);
	UpdateBalls(aGame, aDeltaTime);
	CheckCollisions(aGame);
}

void StressTest::DrawUI(Game& aGame, float aDeltaTime)
{
	std::lock_guard imguiLock(aGame.GetImGUISystem().GetMutex());
	if (ImGui::Begin("Stress Test"))
	{
		ImGui::InputFloat("Spawn Rate", &mySpawnRate);
		myWipeEverything = ImGui::InputScalar("Spawn Square Side", ImGuiDataType_U32, &mySpawnSquareSide);
		mySpawnSquareSide = glm::max(mySpawnSquareSide, 1u);
		ImGui::InputFloat("Tank Speed", &myTankSpeed);
		ImGui::Text("Tanks alive: %llu", myTanks.GetCount());
		ImGui::Checkbox("Draw Shapes", &myDrawShapes);
		ImGui::Checkbox("Control Camera", &myControlCamera);

		ImGui::Separator();

		ImGui::InputFloat("Shoot Cooldown", &myShootCD);
		ImGui::InputFloat("Shot Life", &myShotLife);
		ImGui::InputFloat("Shot Speed", &myShotSpeed);
		ImGui::Text("Cannonballs: %llu", myBalls.GetCount());

		std::rotate(
			std::begin(myDeltaHistory), 
			std::begin(myDeltaHistory) + 1, 
			std::end(myDeltaHistory)
		);
		myDeltaHistory[kHistorySize - 1] = aDeltaTime * 1000;

		ImGui::PlotLines("Delta(ms)", myDeltaHistory, kHistorySize, 0, nullptr);
	}
	ImGui::End();
}

void StressTest::UpdateTanks(Game& aGame, float aDeltaTime)
{
	const float halfHeight = myTankShape->GetHalfExtents().y;
	const Terrain& terrain = *aGame.GetTerrain(0);
	const glm::vec2 halfTerrainSize{ terrain.GetWidth() / 2.f };

	myTankAccum += mySpawnRate * aDeltaTime;
	if (myTankAccum >= myTanks.GetCount() + 1)
	{
		Profiler::ScopedMark scope("SpawnTanks");
		std::uniform_real_distribution<float> filter(
			mySpawnSquareSide / -2.f + 1, // avoid spawning on terrain border
			mySpawnSquareSide / 2.f - 1
		);
		const uint32_t newCount = static_cast<uint32_t>(myTankAccum);
		const uint32_t toSpawnCount = static_cast<uint32_t>(newCount - myTanks.GetCount());
		myTanks.Reserve(newCount);
		for (uint32_t i = 0; i < toSpawnCount; i++)
		{
			myTankSwitch = !myTankSwitch;

			Tank& tank = myTanks.Allocate();

			tank.myTeam = myTankSwitch;
			tank.myCooldown = myShootCD;

			const float xSpawn = (mySpawnSquareSide - 1) / 2.f * (tank.myTeam ? 1 : -1);
			const float zSpawn = filter(myRandEngine);
			const float xTarget = -xSpawn;
			const float zTarget = filter(myRandEngine);
			tank.myDest = glm::vec3(xTarget, 0, zTarget);

			glm::vec2 terrainPos{ xSpawn, zSpawn };
			terrainPos += halfTerrainSize;
			const float terrainHeight = terrain.GetHeight(terrainPos);

			Transform tankTransf;
			tankTransf.SetPos({ xSpawn, terrainHeight, zSpawn });
			tankTransf.SetScale({ kTankScale, kTankScale, kTankScale }); // the model is too large
			tankTransf.LookAt(tank.myDest);

			tank.myGO = new GameObject(tankTransf);
			VisualComponent* visualComp = tank.myGO->AddComponent<VisualComponent>();
			visualComp->SetModel(myTankModel);
			visualComp->SetTextureCount(1);
			visualComp->SetTexture(0, tank.myTeam ? myGreenTankTexture : myRedTankTexture);
			visualComp->SetPipeline(myDefaultPipeline);

			tankTransf.SetScale({ 1, 1, 1 }); // tank has already scaled collider
			Shapes::AABB bounds = myTankShape->GetAABB(tankTransf.GetMatrix());
			myGrid.Add(
				{ bounds.myMin.x, bounds.myMin.z },
				{ bounds.myMax.x, bounds.myMax.z },
				{ &tank, true }
			);

			aGame.AddGameObject(tank.myGO);
		}
	}

	{
		Profiler::ScopedMark scope("MoveTanks");

		size_t removed = 0;
		myTanks.ForEach([&](Tank& aTank)
		{
			Transform transf = aTank.myGO->GetWorldTransform();
			Transform unscaled = transf;
			unscaled.SetScale({ 1, 1, 1 });

			const Shapes::AABB origTankAABB = myTankShape->GetAABB(unscaled.GetMatrix());

			glm::vec3 dist = aTank.myDest - transf.GetPos();
			dist.y = 0;
			const float sqrlength = glm::length2(dist);

			if (sqrlength <= 1.f)
			{
				aGame.RemoveGameObject(aTank.myGO);

				myGrid.Remove(
					{ origTankAABB.myMin.x, origTankAABB.myMin.z },
					{ origTankAABB.myMax.x, origTankAABB.myMax.z },
					{ &aTank, true }
				);
				myTanks.Free(aTank);
				removed++;
				return;
			}

			glm::vec3 tankPos = transf.GetPos();
			tankPos += transf.GetForward() * myTankSpeed * aDeltaTime;

			glm::vec2 terrainPos{ tankPos.x, tankPos.z };
			terrainPos += halfTerrainSize;
			tankPos.y = terrain.GetHeight(terrainPos);
			transf.SetPos(tankPos);
			
			aTank.myGO->SetWorldTransform(transf);

			transf.SetScale({ 1, 1, 1 });
			const Shapes::AABB newTankAABB = myTankShape->GetAABB(transf.GetMatrix());
			if (myDrawShapes)
			{
				const float halfHeight = myTankShape->GetHalfExtents().y;
				const glm::vec3 tankMin = newTankAABB.myMin + glm::vec3{ 0, halfHeight, 0 };
				const glm::vec3 tankMax = newTankAABB.myMax + glm::vec3{ 0, halfHeight, 0 };
				aGame.GetDebugDrawer().AddAABB(tankMin, tankMax, { 0, 1, 0 });
			}
			myGrid.Move(
				{ origTankAABB.myMin.x, origTankAABB.myMin.z },
				{ origTankAABB.myMax.x, origTankAABB.myMax.z },
				{ newTankAABB.myMin.x, newTankAABB.myMin.z },
				{ newTankAABB.myMax.x, newTankAABB.myMax.z },
				{ &aTank, true }
			);

			aTank.myCooldown -= aDeltaTime;
			if (aTank.myCooldown < 0)
			{
				aTank.myCooldown += myShootCD;

				Transform ballTransf;
				ballTransf.SetPos(transf.GetPos()
					+ transf.GetForward() * 0.2f
					+ transf.GetUp() * 0.85f
				);
				ballTransf.SetScale({ kBallScale, kBallScale, kBallScale });

				Ball& ball = myBalls.Allocate();
				ball.myLife = myShotLife;
				ball.myTeam = aTank.myTeam;

				glm::vec3 initVelocity = transf.GetForward();
				initVelocity.y += 0.5f;
				ball.myVel = glm::normalize(initVelocity) * myShotSpeed;

				ball.myGO = new GameObject(ballTransf);
				VisualComponent* visualComp = ball.myGO->AddComponent<VisualComponent>();
				visualComp->SetModel(mySphereModel);
				visualComp->SetTextureCount(1);
				visualComp->SetTexture(0, myGreyTexture);
				visualComp->SetPipeline(myDefaultPipeline);

				ballTransf.SetScale({ 1, 1, 1 });
				Shapes::AABB ballAABB = myBallShape->GetAABB(ballTransf.GetMatrix());
				myGrid.Add(
					{ ballAABB.myMin.x, ballAABB.myMin.z },
					{ ballAABB.myMax.x, ballAABB.myMax.z },
					{ &ball, false }
				);
				aGame.AddGameObject(ball.myGO);

			}
		});
		myTankAccum -= removed;	
	}
}

void StressTest::UpdateBalls(Game& aGame, float aDeltaTime)
{
	const float halfHeight = myTankShape->GetHalfExtents().y;

	{
		Profiler::ScopedMark scope("MoveBalls");
		constexpr static float kGravity = 9.8f;
		myBalls.ForEach([aDeltaTime, &aGame, this](Ball& aBall)
		{
			Transform transf = aBall.myGO->GetWorldTransform();
			Transform unscaled = transf;
			unscaled.SetScale({ 1, 1, 1 });
			Shapes::AABB originalAABB = myBallShape->GetAABB(transf.GetMatrix());

			aBall.myLife -= aDeltaTime;
			if (aBall.myLife < 0)
			{
				myGrid.Remove(
					{ originalAABB.myMin.x, originalAABB.myMin.z },
					{ originalAABB.myMax.x, originalAABB.myMax.z },
					{ &aBall, false }
				);

				aGame.RemoveGameObject(aBall.myGO);
				aBall.myGO = Handle<GameObject>();
				myBalls.Free(aBall);
				return;
			}

			aBall.myVel.y -= kGravity * aDeltaTime;
			transf.Translate(aBall.myVel * aDeltaTime);
			aBall.myGO->SetWorldTransform(transf);

			transf.SetScale({ 1, 1, 1 });
			Shapes::AABB newAABB = myBallShape->GetAABB(transf.GetMatrix());
			if (myDrawShapes)
			{
				aGame.GetDebugDrawer().AddAABB(newAABB.myMin, newAABB.myMax, { 1, 0, 0 });
			}
			myGrid.Move(
				{ originalAABB.myMin.x, originalAABB.myMin.z },
				{ originalAABB.myMax.x, originalAABB.myMax.z },
				{ newAABB.myMin.x, newAABB.myMin.z },
				{ newAABB.myMax.x, newAABB.myMax.z },
				{ &aBall, false }
			);
		});
	}
}

void StressTest::UpdateCamera(Camera& aCam, float aDeltaTime)
{
	Transform& camTransf = aCam.GetTransform();

	if (!myControlCamera)
	{
		constexpr float kSpeed = glm::radians(10.f);
		myRotationAngle += kSpeed * aDeltaTime;
		if (myRotationAngle > glm::two_pi<float>())
		{
			myRotationAngle -= glm::two_pi<float>();
		}

		constexpr glm::vec3 kTargetPos{ 0, 0, 0 };
		const glm::vec3 initPos{ mySpawnSquareSide, mySpawnSquareSide / 2.f, 0 };
		const glm::vec3 pos = Transform::RotateAround(
			initPos,
			{ 0, 0, 0 },
			{ 0, myRotationAngle, 0 }
		);

		camTransf.SetPos(pos);
		camTransf.LookAt(kTargetPos);
	}
	else
	{
		// TODO: consolidate with EditorMode
		glm::vec3 pos = camTransf.GetPos();
		glm::vec3 forward = camTransf.GetForward();
		glm::vec3 right = camTransf.GetRight();
		glm::vec3 up = camTransf.GetUp();
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
		camTransf.SetPos(pos);

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
		constexpr float mouseSens = 0.1f;

		glm::vec3 pitchDelta(mouseDelta.y, 0.f, 0.f);
		pitchDelta *= mouseSens;
		const glm::quat pitchRot(glm::radians(pitchDelta));

		glm::vec3 yawDelta(0.f, -mouseDelta.x, 0.f);
		yawDelta *= mouseSens;
		const glm::quat yawRot(glm::radians(yawDelta));

		camTransf.SetRotation(yawRot * camTransf.GetRotation() * pitchRot);
	}
}

void StressTest::CheckCollisions(Game& aGame)
{
	Profiler::ScopedMark scope("CheckCollisions");
	uint16_t tanksRemoved = 0;

	struct ToRemove
	{
		Shapes::Rect myRect;
		void* myPtr;
		bool myIsTank;
	};
	std::vector<ToRemove> removeQueue;
	removeQueue.reserve(512);

	const float halfHeight = myTankShape->GetHalfExtents().y;
	myGrid.ForEachCell([this, halfHeight, &removeQueue, &aGame](std::vector<TankOrBall>& aCell) 
	{
		if (aCell.size() <= 2)
		{
			return; // nothing to do
		}
		for (uint32_t ballInd = 0; ballInd < aCell.size() - 1; ballInd++)
		{
			if (aCell[ballInd].myIsTank)
			{
				continue;
			}
			Ball* ball = static_cast<Ball*>(aCell[ballInd].myPtr);
			if (!ball->myGO.IsValid() || !ball->myGO->GetWorld())
			{
				continue;
			}

			Transform ballTransf = ball->myGO->GetWorldTransform();
			ballTransf.SetScale({ 1, 1, 1 });
			Shapes::AABB ballAABB = myBallShape->GetAABB(ballTransf.GetMatrix());
			for (uint32_t tankInd = ballInd + 1; tankInd < aCell.size(); tankInd++)
			{
				if (!aCell[tankInd].myIsTank)
				{
					continue;
				}
				Tank* tank = static_cast<Tank*>(aCell[tankInd].myPtr);
				if (!tank->myGO.IsValid() || !tank->myGO->GetWorld())
				{
					continue;
				}

				if (ball->myTeam == tank->myTeam)
				{
					continue;
				}

				Transform tankTransf = tank->myGO->GetWorldTransform();
				tankTransf.SetScale({ 1, 1, 1 });
				tankTransf.Translate({ 0, halfHeight, 0 });
				Shapes::AABB tankAABB = myTankShape->GetAABB(tankTransf.GetMatrix());

				// coarse check
				if (Shapes::Intersects(ballAABB, tankAABB))
				{
					// found it - schedule removal
					const glm::vec2 tankMin{ tankAABB.myMin.x, tankAABB.myMin.z };
					const glm::vec2 tankMax{ tankAABB.myMax.x, tankAABB.myMax.z };
					removeQueue.push_back({ { tankMin, tankMax }, tank, true });
					const glm::vec2 ballMin{ ballAABB.myMin.x, ballAABB.myMin.z };
					const glm::vec2 ballMax{ ballAABB.myMax.x, ballAABB.myMax.z };
					removeQueue.push_back({ { ballMin, ballMax }, ball, false });

					// remove from world
					aGame.RemoveGameObject(tank->myGO);
					aGame.RemoveGameObject(ball->myGO);

					// also mark they're dead
					tank->myGO = {};
					ball->myGO = {};

					// this ball got consumed, so go for next one
					break;
				}
			}
		}
	});

	{
		Profiler::ScopedMark scope("RemoveObjects");
		for (const ToRemove& toRemove : removeQueue)
		{
			myGrid.Remove(
				toRemove.myRect.myMin, toRemove.myRect.myMax,
				{ toRemove.myPtr, toRemove.myIsTank }
			);
			if (toRemove.myIsTank)
			{
				tanksRemoved++;
				myTanks.Free(*static_cast<Tank*>(toRemove.myPtr));
			}
			else
			{
				myBalls.Free(*static_cast<Ball*>(toRemove.myPtr));
			}
		}
	}
	
	myTankAccum -= tanksRemoved;
}

void StressTest::WipeEverything(Game& aGame)
{
	myGrid.Clear();
	myTanks.ForEach([&](Tank& aTank) {
		aGame.RemoveGameObject(aTank.myGO);
	});
	myTanks.Clear();
	myBalls.ForEach([&](Ball& aBall) {
		aGame.RemoveGameObject(aBall.myGO);
	});
	myBalls.Clear();
	myTankAccum = 0;
}

void StressTest::CreateTerrain(Game& aGame, uint32_t aSize)
{
	Terrain* terrain = new Terrain();
	terrain->Generate({ aSize, aSize }, 1, 3);

	terrain->PushHeightLevelColor(0.f, { 0, 0, 1 });
	terrain->PushHeightLevelColor(1.f, { 0, 1, 1 });
	terrain->PushHeightLevelColor(2.f, { 1, 0, 1 });
	terrain->PushHeightLevelColor(3.f, { 1, 1, 1 });

	AssetTracker& assetTracker = aGame.GetAssetTracker();
	Handle<Pipeline> terrainPipeline = assetTracker.GetOrCreate<Pipeline>("TestTerrain/terrain.ppl");
	aGame.AddTerrain(terrain, terrainPipeline);
}