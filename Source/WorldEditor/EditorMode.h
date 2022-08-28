#pragma once

#include <Engine/Animation/Skeleton.h>
#include <Engine/Animation/AnimationController.h>
#include <Engine/Animation/AnimationClip.h>
#include <Core/Pool.h>
#include <Engine/Resources/GLTFImporter.h>
#include <Engine/UIWidgets/FileDialog.h>

#include "HexSolver.h"
#include "DefaultAssets.h"
#include "Gizmos.h"

class PhysicsWorld;
class GameObject;
class PhysicsShapeBase;
class Transform;
class Game;
class AnimationSystem;
class AnimationTest;
class IDRenderPass;
class Terrain;

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
	void DrawMenu(Game& aGame);
	void CreatePlane(Game& aGame);
	void CreateSphere(Game& aGame);
	void CreateBox(Game& aGame);
	void CreateCylinder(Game& aGame);
	void CreateCone(Game& aGame);
	void CreateMesh(Game& aGame);
	
	DefaultAssets myDefAssets;
	std::function<void(Game& aGame)> myMenuFunction;
	FileDialog myFileDialog;

	Gizmos myGizmos;
	
	// Object Picking
	void UpdatePickedObject(Game& aGame);

	IDRenderPass* myIDRenderPass; // non-owning
	GameObject* myPickedGO = nullptr;
	Terrain* myPickedTerrain = nullptr;

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