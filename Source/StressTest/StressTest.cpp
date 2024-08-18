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
#include <Graphics/Utils.h>

#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsShapes.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Debug/DebugDrawer.h>

class TriggersTracker final : public PhysicsWorld::ISymCallbackListener
{
public:
	TriggersTracker(StressTest& aOwner) : myOwner(aOwner) {}

private:
	void OnPrePhysicsStep(float aDeltaTime) override {};
	void OnPostPhysicsStep(float aDeltaTime) override {};

	void OnTriggerCallback(const PhysicsEntity& aLeft, const PhysicsEntity& aRight) override
	{
#if !defined(ST_QTREE) && !defined(ST_GRID)
		// TODO: this is (probably)horribly inefficient, but other than
		// keeping track of indices on PhysicsEntity (which I 
		// can't do - no stability guarantee) I got no ideas
		auto tankIter = std::find_if(myOwner.myTanks.begin(), myOwner.myTanks.end(), 
			[left = &aLeft, right = &aRight](const StressTest::Tank& aTank) {
				return aTank.myTrigger == left || aTank.myTrigger == right;
			}
		);
		if (tankIter == myOwner.myTanks.end())
		{
			return;
		}

		auto ballIter = std::find_if(myOwner.myBalls.begin(), myOwner.myBalls.end(),
			[left = &aLeft, right = &aRight](const StressTest::Ball& aBall) {
				return aBall.myTrigger == left || aBall.myTrigger == right;
			}
		);
		if (ballIter == myOwner.myBalls.end())
		{
			return;
		}

		StressTest::Tank& tank = *tankIter;
		StressTest::Ball& ball = *ballIter;
		if (tank.myTeam != ball.myTeam)
		{
			// we have to defer destruction as we don't
			// have access to the Game
			myOwner.myTanksToRemove.push_back(tank);
			myOwner.myBallsToRemove.push_back(ball);

			auto lastTankIter = myOwner.myTanks.end();
			std::advance(lastTankIter, -1);
			if (tankIter != lastTankIter)
			{
				std::swap(tank, myOwner.myTanks.back());
			}
			myOwner.myTanks.erase(lastTankIter);
			myOwner.myTankAccum--;

			auto lastBallIter = myOwner.myBalls.end();
			std::advance(lastBallIter, -1);
			if (ballIter != lastBallIter)
			{
				std::swap(ball, myOwner.myBalls.back());
			}
			myOwner.myBalls.erase(lastBallIter);
		}
#endif
	}

	StressTest& myOwner;
};

StressTest::StressTest(Game& aGame)
#ifdef ST_QTREE
	: myTanksTree({ -200, -200 }, {200, 200}, 8) // maxDepth is arbitrary
	// TODO: add support for resizing
#elif defined(ST_GRID)
	: myGrid({ -200, -200 }, 400, 50)
#endif
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

#ifndef ST_STABLE
	myTanks.reserve(500);
	myBalls.reserve(500);
#endif

	myTriggersTracker = new TriggersTracker(*this);
	aGame.GetWorld().CreatePhysWorld();
	aGame.GetWorld().GetPhysicsWorld()->AddPhysSystem(myTriggersTracker);

	myLight = aGame.GetLightSystem().AllocateLight();
	Light& light = *myLight.Get();
	light.myType = Light::Type::Directional;
	light.myColor = { 1, 1, 1 };
	light.myAmbientIntensity = 0.9f;
	light.myTransform.LookAt({ 0, -10, 0 });
}

StressTest::~StressTest()
{
	delete myTriggersTracker;
}

void StressTest::Update(Game& aGame, float aDeltaTime)
{
	Profiler::ScopedMark mark("StressTest::Update");
	DrawUI(aGame, aDeltaTime);
#ifdef ST_QTREE
	if (myDrawQuadTree)
	{
		DebugDrawer& debugDrawer = aGame.GetDebugDrawer();
		constexpr glm::vec3 kDepthColors[]{
			{1, 0, 0},
			{0, 1, 0},
			{0, 0, 1}
		};
		myTanksTree.ForEachQuad([&](glm::vec2 aMin, glm::vec2 aMax, uint8_t aDepth, std::span<Tank*>)
		{
			const glm::vec3 min{ aMin.x, 0, aMin.y };
			const glm::vec3 max{ aMax.x, 0, aMax.y };
			const glm::vec3 color = kDepthColors[aDepth % std::size(kDepthColors)];
			debugDrawer.AddLine(min, { min.x, 0, max.z }, color);
			debugDrawer.AddLine(min, { max.x, 0, min.z }, color);
			debugDrawer.AddLine({ min.x, 0, max.z }, max, color);
			debugDrawer.AddLine({ max.x, 0, min.z }, max, color);
		});
	}
#endif
	if (aGame.IsPaused())
	{
		return;
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
		ImGui::InputFloat("Spawn Square Side", &mySpawnSquareSide);
		mySpawnSquareSide = glm::max(mySpawnSquareSide, 1.f);
		ImGui::InputFloat("Tank Speed", &myTankSpeed);
#ifdef ST_STABLE
		ImGui::Text("Tanks alive: %llu", myTanks.GetCount());
#else
		ImGui::Text("Tanks alive: %llu", myTanks.size());
#endif
		ImGui::Checkbox("Draw Shapes", &myDrawShapes);
#ifdef ST_QTREE
		ImGui::Checkbox("Draw QuadTree", &myDrawQuadTree);
#endif

		ImGui::Separator();

		ImGui::InputFloat("Shoot Cooldown", &myShootCD);
		ImGui::InputFloat("Shot Life", &myShotLife);
		ImGui::InputFloat("Shot Speed", &myShotSpeed);
#ifdef ST_STABLE
		ImGui::Text("Cannonballs: %llu", myBalls.GetCount());
#else
		ImGui::Text("Cannonballs: %llu", myBalls.size());
#endif

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
	for (Tank& tank : myTanksToRemove)
	{
		aGame.RemoveGameObject(tank.myGO);
	}
	myTanksToRemove.clear();

	const float halfHeight = myTankShape->GetHalfExtents().y;

	myTankAccum += mySpawnRate * aDeltaTime;
#ifdef ST_STABLE
	if (myTankAccum >= myTanks.GetCount() + 1)
#else
	if (myTankAccum >= myTanks.size() + 1)
#endif
	{
		Profiler::ScopedMark scope("SpawnTanks");
		std::uniform_real_distribution<float> filter(
			-mySpawnSquareSide / 2.f,
			mySpawnSquareSide / 2.f
		);
		const uint32_t newCount = static_cast<uint32_t>(myTankAccum);
#ifdef ST_STABLE
		const uint32_t toSpawnCount = static_cast<uint32_t>(newCount - myTanks.GetCount());
		myTanks.Reserve(newCount);
#else
		const uint32_t toSpawnCount = static_cast<uint32_t>(newCount - myTanks.size());
		myTanks.resize(newCount);
#endif
		for (uint32_t i = 0; i < toSpawnCount; i++)
		{
			myTankSwitch = !myTankSwitch;

#ifdef ST_STABLE
			Tank& tank = myTanks.Allocate();
#else
			Tank& tank = myTanks[newCount - i - 1];
#endif

			tank.myTeam = myTankSwitch;
			tank.myCooldown = myShootCD;

			const float xSpawn = mySpawnSquareSide / 2.f * (tank.myTeam ? 1 : -1);
			const float zSpawn = filter(myRandEngine);
			const float xTarget = -xSpawn;
			const float zTarget = filter(myRandEngine);
			tank.myDest = glm::vec3(xTarget, 0, zTarget);

			Transform tankTransf;
			tankTransf.SetPos({ xSpawn, 0, zSpawn });
			tankTransf.SetScale({ kTankScale, kTankScale, kTankScale }); // the model is too large
			tankTransf.LookAt(tank.myDest);

			tank.myGO = new GameObject(tankTransf);
			VisualComponent* visualComp = tank.myGO->AddComponent<VisualComponent>();
			visualComp->SetModel(myTankModel);
			visualComp->SetTextureCount(1);
			visualComp->SetTexture(0, tank.myTeam ? myGreenTankTexture : myRedTankTexture);
			visualComp->SetPipeline(myDefaultPipeline);

#ifdef ST_QTREE
			tankTransf.SetScale({ 1, 1, 1 }); // tank has already scaled collider
			AABB bounds = myTankShape->GetAABB(tankTransf.GetMatrix());
			tank.myTreeInfo = myTanksTree.Add(
				{ bounds.myMin.x, bounds.myMin.z },
				{ bounds.myMax.x, bounds.myMax.z },
				&tank
			);
#elif defined(ST_GRID)
			tankTransf.SetScale({ 1, 1, 1 }); // tank has already scaled collider
			AABB bounds = myTankShape->GetAABB(tankTransf.GetMatrix());
			myGrid.Add(
				{ bounds.myMin.x, bounds.myMin.z },
				{ bounds.myMax.x, bounds.myMax.z },
				{ &tank, true }
			);
#else
			PhysicsComponent* physComp = tank.myGO->AddComponent<PhysicsComponent>();
			const float halfHeight = myTankShape->GetHalfExtents().y / 2.f;
			physComp->CreateTriggerEntity(myTankShape, { 0, halfHeight, 0 });
			physComp->RequestAddToWorld(*aGame.GetWorld().GetPhysicsWorld());
			tank.myTrigger = &physComp->GetPhysicsEntity();
#endif

			aGame.AddGameObject(tank.myGO);
		}
	}

	{
		Profiler::ScopedMark scope("MoveTanks");

#ifdef ST_STABLE
		size_t removed = 0;
		myTanks.ForEach([&](Tank& tank) // TODO: fix the naming!
#else
		for (Tank& tank : myTanks)
#endif
		{
#ifdef ST_QTREE
			if (!tank.myGO.IsValid())
			{
				myTanksTree.Remove(tank.myTreeInfo, &tank);
				myTanks.Free(tank);
				removed++;
				return;
			}
#endif

			Transform transf = tank.myGO->GetWorldTransform();
#ifdef ST_GRID
			Transform unscaled = transf;
			unscaled.SetScale({ 1, 1, 1 });
			const AABB origTankAABB = myTankShape->GetAABB(unscaled.GetMatrix());
#endif
			glm::vec3 dist = tank.myDest - transf.GetPos();
			dist.y = 0;
			const float sqrlength = glm::length2(dist);

			if (sqrlength <= 1.f)
			{
				// DEBUG
				ASSERT(tank.myGO->GetWorld());
				aGame.RemoveGameObject(tank.myGO);

#ifdef ST_STABLE
#ifdef ST_QTREE
				myTanksTree.Remove(tank.myTreeInfo, &tank);
#elif defined(ST_GRID)
				myGrid.Remove(
					{ origTankAABB.myMin.x, origTankAABB.myMin.z },
					{ origTankAABB.myMax.x, origTankAABB.myMax.z },
					{ &tank, true }
				);
#endif
				myTanks.Free(tank);
				removed++;
				return;
#else
				tank.myGO = Handle<GameObject>();
				continue;
#endif
			}

			transf.Translate(transf.GetForward() * myTankSpeed * aDeltaTime);
			tank.myGO->SetWorldTransform(transf);

#ifdef ST_QTREE
			transf.Translate({ 0, halfHeight, 0 });
			transf.SetScale({ 1, 1, 1 });
			AABB tankAABB = myTankShape->GetAABB(transf.GetMatrix());
			if (myDrawShapes)
			{
				aGame.GetDebugDrawer().AddAABB(tankAABB.myMin, tankAABB.myMax, { 0, 1, 0 });
			}
			tank.myTreeInfo = myTanksTree.Move(
				{ tankAABB.myMin.x, tankAABB.myMin.z },
				{ tankAABB.myMax.x, tankAABB.myMax.z },
				tank.myTreeInfo, &tank
			);
#elif defined(ST_GRID)
			transf.SetScale({ 1, 1, 1 });
			const AABB newTankAABB = myTankShape->GetAABB(transf.GetMatrix());
			if (myDrawShapes)
			{
				aGame.GetDebugDrawer().AddAABB(newTankAABB.myMin, newTankAABB.myMax, { 0, 1, 0 });
			}
			myGrid.Move(
				{ origTankAABB.myMin.x, origTankAABB.myMin.z },
				{ origTankAABB.myMax.x, origTankAABB.myMax.z },
				{ newTankAABB.myMin.x, newTankAABB.myMin.z },
				{ newTankAABB.myMax.x, newTankAABB.myMax.z },
				{ &tank, true }
			);
#endif

			tank.myCooldown -= aDeltaTime;
			if (tank.myCooldown < 0)
			{
				tank.myCooldown += myShootCD;

				Transform ballTransf;
				ballTransf.SetPos(transf.GetPos()
					+ transf.GetForward() * 0.2f
					+ transf.GetUp() * 0.85f
				);
				ballTransf.SetScale({ kBallScale, kBallScale, kBallScale });

#ifdef ST_STABLE
				Ball& ball = myBalls.Allocate();
#else
				Ball ball;
#endif
				ball.myLife = myShotLife;
				ball.myTeam = tank.myTeam;

				glm::vec3 initVelocity = transf.GetForward();
				initVelocity.y += 0.5f;
				ball.myVel = glm::normalize(initVelocity) * myShotSpeed;

				ball.myGO = new GameObject(ballTransf);
				VisualComponent* visualComp = ball.myGO->AddComponent<VisualComponent>();
				visualComp->SetModel(mySphereModel);
				visualComp->SetTextureCount(1);
				visualComp->SetTexture(0, myGreyTexture);
				visualComp->SetPipeline(myDefaultPipeline);

#if !defined(ST_QTREE) && !defined(ST_GRID)
				const float halfHeight = myBallShape->GetRadius();
				PhysicsComponent* physComp = ball.myGO->AddComponent<PhysicsComponent>();
				physComp->CreateTriggerEntity(myBallShape, { 0, halfHeight, 0 });
				physComp->RequestAddToWorld(*aGame.GetWorld().GetPhysicsWorld());
				ball.myTrigger = &physComp->GetPhysicsEntity();
#endif

#ifdef ST_GRID
				ballTransf.SetScale({ 1, 1, 1 });
				AABB ballAABB = myBallShape->GetAABB(ballTransf.GetMatrix());
				myGrid.Add(
					{ ballAABB.myMin.x, ballAABB.myMin.z },
					{ ballAABB.myMax.x, ballAABB.myMax.z },
					{ &ball, false }
				);
#endif
				aGame.AddGameObject(ball.myGO);

#ifndef ST_STABLE
				myBalls.push_back(ball);
#endif
			}
		}
#ifdef ST_STABLE
		);
#endif

		// CleanUp
#ifndef ST_STABLE
		const size_t removed = std::erase_if(myTanks,
			[](const Tank& aTank) {
			return !aTank.myGO.IsValid();
		}
		);
#endif
		myTankAccum -= removed;	
	}
}

void StressTest::UpdateBalls(Game& aGame, float aDeltaTime)
{
	for (Ball& ball : myBallsToRemove)
	{
		aGame.RemoveGameObject(ball.myGO);
	}
	myBallsToRemove.clear();

	const float halfHeight = myTankShape->GetHalfExtents().y;

	{
		Profiler::ScopedMark scope("MoveBalls");
		constexpr static float kGravity = 9.8f;
#ifdef ST_STABLE
		myBalls.ForEach([aDeltaTime, &aGame, this](Ball& ball)
#else
		for (Ball& ball : myBalls)
#endif
		{
			Transform transf = ball.myGO->GetWorldTransform();
#ifdef ST_GRID
			Transform unscaled = transf;
			unscaled.SetScale({ 1, 1, 1 });
			AABB originalAABB = myBallShape->GetAABB(transf.GetMatrix());
#endif

			ball.myLife -= aDeltaTime;
			if (ball.myLife < 0)
			{
#ifdef ST_GRID
				myGrid.Remove(
					{ originalAABB.myMin.x, originalAABB.myMin.z },
					{ originalAABB.myMax.x, originalAABB.myMax.z },
					{ &ball, false }
				);
#endif

				aGame.RemoveGameObject(ball.myGO);
				ball.myGO = Handle<GameObject>();
#ifdef ST_STABLE
				myBalls.Free(ball);
				return;
#else
				continue;
#endif
			}


			ball.myVel.y -= kGravity * aDeltaTime;
			transf.Translate(ball.myVel * aDeltaTime);
			ball.myGO->SetWorldTransform(transf);

#ifdef ST_GRID
			transf.SetScale({ 1, 1, 1 });
			AABB newAABB = myBallShape->GetAABB(transf.GetMatrix());
			if (myDrawShapes)
			{
				aGame.GetDebugDrawer().AddAABB(newAABB.myMin, newAABB.myMax, { 1, 0, 0 });
			}
			myGrid.Move(
				{ originalAABB.myMin.x, originalAABB.myMin.z },
				{ originalAABB.myMax.x, originalAABB.myMax.z },
				{ newAABB.myMin.x, newAABB.myMin.z },
				{ newAABB.myMax.x, newAABB.myMax.z },
				{ &ball, false }
			);
#endif

#ifdef ST_QTREE
			if (!ball.myGO->GetWorld())
			{
				continue;
			}

			// Doing manual collision resolution
			transf.SetScale({ 1, 1, 1 });
			AABB aabb = myBallShape->GetAABB(transf.GetMatrix());
			if (myDrawShapes)
			{
				aGame.GetDebugDrawer().AddAABB(aabb.myMin, aabb.myMax, { 1, 0, 0 });
			}
			Utils::AABB translatedAABB{ aabb.myMin, aabb.myMax };
			Profiler::ScopedMark scope("TestBall");
			myTanksTree.Test(
				{ aabb.myMin.x, aabb.myMin.z },
				{ aabb.myMax.x, aabb.myMax.z },
				[&](Tank* aTank)
			{
				if (ball.myTeam == aTank->myTeam)
				{
					return true;
				}

				if (!aTank->myGO.IsValid()
					|| !aTank->myGO->GetWorld()) [[unlikely]]
				{
					return true;
				}

				Transform tankTransf = aTank->myGO->GetWorldTransform();
				tankTransf.SetScale({ 1, 1, 1 });
				tankTransf.Translate({ 0, halfHeight, 0 });
				AABB tankAABB = myTankShape->GetAABB(tankTransf.GetMatrix());
				// coarse check
				Utils::AABB translatedTankAABB{ tankAABB.myMin, tankAABB.myMax };
				if (!Utils::Intersects(translatedAABB, translatedTankAABB))
				{
					return true;
				}

				// TODO: fine check

				// found it - destroy it and self
				ASSERT(aTank->myGO->GetWorld());
				aGame.RemoveGameObject(aTank->myGO);
				aGame.RemoveGameObject(ball.myGO);
				aTank->myGO = {};
				ball.myGO = {};
				return false;
			});
#endif
		}
#ifdef ST_STABLE
			);
#endif
	}

#ifndef ST_STABLE
	Profiler::ScopedMark scope("RemoveBalls");
	std::erase_if(myBalls,
		[](const Ball& aBall) {
			return !aBall.myGO.IsValid();
		}
	);
#endif
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

void StressTest::CheckCollisions(Game& aGame)
{
	Profiler::ScopedMark scope("CheckCollisions");
	uint16_t tanksRemoved = 0;

#ifdef ST_GRID
	struct ToRemove
	{
		AABB myAABB;
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
			AABB ballAABB = myBallShape->GetAABB(ballTransf.GetMatrix());
			const Utils::AABB translatedBallAABB{ ballAABB.myMin, ballAABB.myMax };
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
				AABB tankAABB = myTankShape->GetAABB(tankTransf.GetMatrix());

				const Utils::AABB translatedTankAABB{ tankAABB.myMin, tankAABB.myMax };
				// coarse check
				if (Utils::Intersects(translatedBallAABB, translatedTankAABB))
				{
					// found it - schedule removal
					// TODO: use vec2!
					removeQueue.push_back({ tankAABB, tank, true });
					removeQueue.push_back({ ballAABB, ball, false });

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
				{ toRemove.myAABB.myMin.x, toRemove.myAABB.myMin.z },
				{ toRemove.myAABB.myMax.x, toRemove.myAABB.myMax.z },
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
#endif
	
	myTankAccum -= tanksRemoved;
}