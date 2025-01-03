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
#include "GameProto.h"
#include "QuadTreeTest.h"
#include "GridTest.h"

class GameObject;
class Transform;
class Game;
class AnimationSystem;
class AnimationTest;
class IDRenderPass;
class GrassRenderPass;
class Terrain;
struct Light;
class NavMeshGen;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(Game& aGame);
	~EditorMode();

	void Update(Game& aGame, float aDeltaTime);

private:
	// Worlds
	void CreateBigWorld(Game& aGame);
	void CreateNavWorld(Game& aGame);

	// Camera
	void HandleCamera(Transform& aCamTransf, float aDeltaTime);
	
	const float myMouseSensitivity = 0.1f;
	float myFlightSpeed = 2.f;

	// Editor UI menu
	void DrawMenu(Game& aGame);
	Handle<GameObject> CreateGOWithMesh(Game& aGame, Handle<Model> aModel, const Transform& aTransf);
	void CreateMesh(Game& aGame, const Transform& aTransf);
	
	DefaultAssets myDefAssets;
	std::function<void(Game& aGame)> myMenuFunction;
	FileDialog myFileDialog;
	std::string myWorldPath;

	Gizmos myGizmos;
	NavMeshGen* myNavMesh = nullptr;
	glm::vec3 myNavMeshOrigin{0, 0, 0};
	glm::vec3 myNavMeshExtents{3, 5, 3};
	float myMaxSlope = glm::radians(45.f);
	bool myDrawGenAABB = false;
	bool myDebugTriangles = false;
	bool myRenderVoxels = false;
	bool myDrawRegions = false;
	bool myDrawCornerPoints = false;
	bool myDrawContours = false;
	
	// Object Picking
	void UpdatePickedObject(Game& aGame);

	IDRenderPass* myIDRenderPass; // non-owning
	GrassRenderPass* myGrassRenderPass; // non-owning
	GameObject* myPickedGO = nullptr;
	Terrain* myPickedTerrain = nullptr;

	// Lights
	void ManageLights(Game& aGame);
	
	std::vector<PoolPtr<Light>> myLights;
	constexpr static size_t kInvalidInd = static_cast<size_t>(-1);
	size_t mySelectedLight = kInvalidInd;

	// Testing
	void AddTestSkeleton(Game& aGame);
	void UpdateTestSkeleton(Game& aGame, float aDeltaTime);
	void DrawBoneHierarchy(int aSkeletonIndex);
	void DrawBoneInfo(int aSkeletonIndex);
	
	Skeleton::BoneIndex mySelectedBone = Skeleton::kInvalidIndex;
	std::vector<GameObject*> myGOs;
	
	int mySelectedSkeleton = -1;
	int myAddSkeletonCount = 0;
	bool myShowSkeletonUI = false;
	AnimationTest* myAnimTest = nullptr;

	struct KeepAlive
	{
		Handle<GameObject> myGO;
		Handle<AnimationClip> myAnimClip;
	};
	std::vector<KeepAlive> myKeepAlives;

	HexSolver solver;
	GameProto myProto;

	GridTest myGridTest;
};