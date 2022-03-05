#include "Precomp.h"
#include "StressTest.h"

#include <Engine/Game.h>
#include <Engine/Terrain.h>
#include <Engine/Resources/OBJImporter.h>
#include <Engine/Components/VisualComponent.h>
#include <Engine/Input.h>

#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Camera.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Debug/DebugDrawer.h>

StressTest::StressTest(Game& aGame)
{
	AssetTracker& assetTracker = aGame.GetAssetTracker();

	{
		constexpr uint32_t kTerrCells = 64;
		
		Terrain* terrain = new Terrain();
		terrain->Generate(glm::ivec2(kTerrCells, kTerrCells), 1, 1);

		Handle<Pipeline> terrainPipeline = assetTracker.GetOrCreate<Pipeline>("TestTerrain/terrain.ppl");
		aGame.AddTerrain(terrain, terrainPipeline);
	}

	Handle<Model> tankModel;
	{
		OBJImporter importer;
		constexpr StaticString path = Resource::kAssetsFolder + "Tank/Tank.obj";
		[[maybe_unused]] bool loadRes = importer.Load(path);
		ASSERT_STR(loadRes, "Failed to load %s", path.CStr());
		tankModel = importer.GetModel(0);
	}

	Handle<Texture> tankTexture = assetTracker.GetOrCreate<Texture>("Tank/playerTank.img");
	Handle<Pipeline> tankPipeline = assetTracker.GetOrCreate<Pipeline>("Engine/default.ppl");

	Transform tankTransf;
	tankTransf.SetPos({ 0, 3, 0 });
	tankTransf.SetScale({ 0.01f, 0.01f, 0.01f });
	myTank = new GameObject(tankTransf);
	VisualComponent* visualComp = myTank->AddComponent<VisualComponent>();
	visualComp->SetModel(tankModel);
	visualComp->SetTextureCount(1);
	visualComp->SetTexture(0, tankTexture);
	visualComp->SetPipeline(tankPipeline);
	aGame.AddGameObject(myTank);
}

void StressTest::Update(Game& aGame, float aDeltaTime)
{
	constexpr float kSpeed = glm::radians(10.f);
	myRotationAngle += kSpeed * aDeltaTime;
	if (myRotationAngle > glm::two_pi<float>())
	{
		myRotationAngle -= glm::two_pi<float>();
	}

	constexpr float kRadius = 10;
	const glm::vec3 initPos{ kRadius, kRadius, 0 };
	const glm::vec3 pos = Transform::RotateAround(
		initPos,
		myTank->GetWorldTransform().GetPos(),
		{ 0, myRotationAngle, 0 }
	);

	Camera& camera = *aGame.GetCamera();
	Transform& camTransf = camera.GetTransform();
	camTransf.SetPos(pos);
	camTransf.LookAt(myTank->GetWorldTransform().GetPos());
}