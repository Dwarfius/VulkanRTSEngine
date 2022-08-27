#pragma once

#include <Engine/Animation/Skeleton.h>
#include <Engine/Animation/AnimationController.h>
#include <Engine/Animation/AnimationClip.h>
#include <Core/Pool.h>
#include <Engine/Resources/GLTFImporter.h>
#include <Engine/UIWidgets/FileDialog.h>

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
	void CreateDefaultResources(Game& aGame);
	void DrawMenu(Game& aGame);
	void CreatePlane(Game& aGame);
	void CreateSphere(Game& aGame);
	void CreateBox(Game& aGame);
	void CreateCylinder(Game& aGame);
	void CreateCone(Game& aGame);
	void CreateMesh(Game& aGame);
	
	Handle<Model> myPlane;
	Handle<Model> mySphere;
	Handle<Model> myBox;
	Handle<Model> myCylinder;
	Handle<Model> myCone;
	Handle<Texture> myUVTexture;
	Handle<Pipeline> myDefaultPipeline;
	std::function<void(Game& aGame)> myMenuFunction;
	FileDialog myFileDialog;

	void DrawGizmos(Game& aGame);
	glm::vec3 myOldMousePosWS;
	uint8_t myPickedAxis = 3;
	
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