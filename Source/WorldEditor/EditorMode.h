#pragma once

#include <Engine/Systems/ProfilerUI.h>
#include <Engine/Animation/Skeleton.h>
#include <Engine/Animation/AnimationController.h>
#include <Engine/Animation/AnimationClip.h>
#include <Core/Pool.h>
#include <Engine/Resources/GLTFImporter.h>
#include <Engine/UIWidgets/TopBar.h>
#include <Engine/UIWidgets/EntitiesWidget.h>
#include <Engine/UIWidgets/ObjImportDialog.h>
#include <Engine/UIWidgets/GltfImportDialog.h>
#include <Engine/UIWidgets/TextureImportDialog.h>
#include <Engine/UIWidgets/ShaderCreateDialog.h>
#include <Engine/UIWidgets/PipelineCreateDialog.h>

class PhysicsWorld;
class GameObject;
class PhysicsShapeBase;
class Transform;
class Game;
class AnimationSystem;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(Game& aGame);

	void Update(Game& aGame, float aDeltaTime, PhysicsWorld* aWorld);

private:
	void HandleCamera(Transform& aCamTransf, float aDeltaTime);

	const float myMouseSensitivity = 0.1f;
	float myFlightSpeed = 2.f;
	bool myDemoWindowVisible = false;
	bool myShowCameraInfo = false;
	bool myShowProfiler = false;
	bool myShowEntitiesView = false;
	bool myShowObjImport = false;
	bool myShowGltfImport = false;
	bool myShowTextureImport = false;
	bool myShowShaderCreate = false;
	bool myShowPipelineCreate = false;

	std::shared_ptr<PhysicsShapeBase> myPhysShape;
	TopBar myTopBar;
	ProfilerUI myProfilerUI;
	EntitiesWidget myEntitiesView;
	ObjImportDialog myObjImport;
	GltfImportDialog myGltfImport;
	TextureImportDialog myTextureImport;
	ShaderCreateDialog myShaderCreate;
	PipelineCreateDialog myPipelineCreate;

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

	Handle<GameObject> myGO;
};