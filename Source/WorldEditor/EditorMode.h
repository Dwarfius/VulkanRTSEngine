#pragma once

#include <Engine/Animation/Skeleton.h>
#include <Engine/Animation/AnimationController.h>
#include <Engine/Animation/AnimationClip.h>
#include <Core/Pool.h>
#include <Engine/Resources/GLTFImporter.h>

#include "HexSolver.h"

class PhysicsWorld;
class GameObject;
class PhysicsShapeBase;
class Transform;
class Game;
class AnimationSystem;
class AnimationTest;
class Model;
class Texture;
class Pipeline;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(Game& aGame);
	~EditorMode();

	void Update(Game& aGame, float aDeltaTime, PhysicsWorld* aWorld);

private:
	// Camera
	void HandleCamera(Transform& aCamTransf, float aDeltaTime);
	
	const float myMouseSensitivity = 0.1f;
	float myFlightSpeed = 2.f;

	// Editor UI menu
	void CreateDefaultResources(Game& aGame);
	void DrawMenu(Game& aGame);
	void CreatePlane(Game& aGame);
	void CreateSphere(Game& aGame);
	void CreateBox(Game& aGame);
	void CreateMesh(Game& aGame);
	
	Handle<Model> myPlane;
	Handle<Model> mySphere;
	Handle<Model> myBox;
	Handle<Texture> myUVTexture;
	Handle<Pipeline> myDefaultPipeline;
	std::function<void(Game& aGame)> myMenuFunction;
	bool myDrawAssets = false;

	struct Asset;
	void DrawAssets(Game& aGame);
	bool GetPickedAsset(Asset& anAsset);

	struct Asset
	{
		std::string myPath;
		std::string myType;

		auto operator<=>(const Asset& aOther) const
		{
			if (auto res = myType <=> aOther.myType; res != 0)
			{
				return res;
			}
			return myPath <=> aOther.myPath;
		}
	};
	std::vector<Asset> myAssets;
	const Asset* mySelectedAsset = nullptr;

	// Other
	std::shared_ptr<PhysicsShapeBase> myPhysShape;

	// Testing
	void AddTestSkeleton(Game& aGame);
	void UpdateTestSkeleton(Game& aGame, float aDeltaTime);
	void DrawBoneHierarchy(int aSkeletonIndex);
	void DrawBoneInfo(int aSkeletonIndex);
	
	Skeleton::BoneIndex mySelectedBone = Skeleton::kInvalidIndex;
	std::vector<GameObject*> myGOs;
	GLTFImporter myGLTFImporter;
	int mySelectedSkeleton = -1;
	int myAddSkeletonCount = 0;
	bool myShowSkeletonUI = false;
	AnimationTest* myAnimTest;

	Handle<GameObject> myGO;

	HexSolver solver;
};