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
	}

	StressTest& myOwner;
};

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

	myTanks.reserve(500);
	myBalls.reserve(500);

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
	if (aGame.IsPaused())
	{
		return;
	}

	UpdateCamera(*aGame.GetCamera(), aDeltaTime);
	UpdateTanks(aGame, aDeltaTime);
	UpdateBalls(aGame, aDeltaTime);
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
		ImGui::Text("Tanks alive: %llu", myTanks.size());

		ImGui::Separator();

		ImGui::InputFloat("Shoot Cooldown", &myShootCD);
		ImGui::InputFloat("Shot Life", &myShotLife);
		ImGui::InputFloat("Shot Speed", &myShotSpeed);
		ImGui::Text("Cannonballs: %llu", myBalls.size());

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
			tankTransf.SetScale({ kTankScale, kTankScale, kTankScale }); // the model is too large
			tankTransf.LookAt(tank.myDest);

			tank.myGO = new GameObject(tankTransf);
			VisualComponent* visualComp = tank.myGO->AddComponent<VisualComponent>();
			visualComp->SetModel(myTankModel);
			visualComp->SetTextureCount(1);
			visualComp->SetTexture(0, tank.myTeam ? myGreenTankTexture : myRedTankTexture);
			visualComp->SetPipeline(myDefaultPipeline);
			
			PhysicsComponent* physComp = tank.myGO->AddComponent<PhysicsComponent>();
			const float halfHeight = myTankShape->GetHalfExtents().y / 2.f;
			physComp->CreateTriggerEntity(myTankShape, {0, halfHeight, 0});
			physComp->RequestAddToWorld(*aGame.GetWorld().GetPhysicsWorld());
			tank.myTrigger = &physComp->GetPhysicsEntity();

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
			ballTransf.SetScale({ kBallScale, kBallScale, kBallScale });

			Ball ball;
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

			const float halfHeight = myBallShape->GetRadius();
			PhysicsComponent* physComp = ball.myGO->AddComponent<PhysicsComponent>();
			physComp->CreateTriggerEntity(myBallShape, { 0, halfHeight, 0 });
			physComp->RequestAddToWorld(*aGame.GetWorld().GetPhysicsWorld());
			ball.myTrigger = &physComp->GetPhysicsEntity();

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
	for (Ball& ball : myBallsToRemove)
	{
		aGame.RemoveGameObject(ball.myGO);
	}
	myBallsToRemove.clear();

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