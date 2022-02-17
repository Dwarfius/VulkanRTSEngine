#include "Precomp.h"
#include "AnimationTest.h"

#include <Engine/Game.h>
#include <Engine/VisualObject.h>
#include <Core/Resources/AssetTracker.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>
#include <Engine/Animation/AnimationSystem.h>
#include <Engine/Animation/AnimationClip.h>
#include <Engine/Resources/GLTFImporter.h>
#include <Engine/Animation/SkinnedVerts.h>

AnimationTest::AnimationTest(Game& aGame)
	: myGame(aGame)
{
	myGO = new GameObject(Transform{});
	myGame.AddGameObject(myGO);
	GameObject* go = myGO.Get();

	PoolPtr<Skeleton> testSkeleton = aGame.GetAnimationSystem().AllocateSkeleton(4);
	Skeleton* skeleton = testSkeleton.Get();

	skeleton->AddBone(Skeleton::kInvalidIndex, {});
	skeleton->AddBone(0, { glm::vec3(0, 1, 0), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->AddBone(1, { glm::vec3(0, 0, 1), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->AddBone(2, { glm::vec3(0, 1, 0), glm::vec3(0.f), glm::vec3(1.f) });

	myClip = new AnimationClip(4, true);
	std::vector<AnimationClip::Mark> marks;
	marks.reserve(5);
	marks.emplace_back(0.f, glm::vec3(0));
	marks.emplace_back(1.f, glm::vec3(1, 0, 0));
	marks.emplace_back(2.f, glm::vec3(0));
	marks.emplace_back(3.f, glm::vec3(-1, 0, 0));
	marks.emplace_back(4.f, glm::vec3(0));
	myClip->AddTrack(0, AnimationClip::Property::Position, AnimationClip::Interpolation::Linear, marks);

	PoolPtr<AnimationController> testController = aGame.GetAnimationSystem().AllocateController(testSkeleton);
	testController.Get()->PlayClip(myClip.Get());
	
	go->SetSkeleton(std::move(testSkeleton));
	go->SetAnimController(std::move(testController));

	AssetTracker& tracker = myGame.GetAssetTracker();
	Handle<Pipeline> skinnedPipeline = tracker.GetOrCreate<Pipeline>("AnimTest/skinned.ppl");
	Handle<Texture> wireframeTexture = tracker.GetOrCreate<Texture>("Engine/wireframe.img");
	VisualObject* vo = new VisualObject();
	vo->SetPipeline(skinnedPipeline);
	vo->SetTexture(wireframeTexture);
	vo->SetModel(GenerateModel(*skeleton));
	go->SetVisualObject(vo);
}

void AnimationTest::Update(float aDeltaTime)
{
	DebugDrawer& drawer = myGame.GetDebugDrawer();
	myGO->GetSkeleton().Get()->DebugDraw(drawer, { glm::vec3(0), glm::vec3(0), glm::vec3(1) });
}

Handle<Model> AnimationTest::GenerateModel(const Skeleton& aSkeleton)
{
	// for each skeleton bone - 1 we need a cube and an arm (rectangular cuboid)
	std::vector<SkinnedVertex> verts;
	std::vector<IModel::IndexType> indices;
	auto appendAACuboid = [&](glm::vec3 aCenter, glm::vec3 aSize, uint16_t aBone)
	{
		IModel::IndexType indexStart = static_cast<IModel::IndexType>(verts.size());
		verts.reserve(verts.size() + 8);

		bool xExtended = aSize.x != aSize.y && aSize.x != aSize.z;
		bool yExtended = aSize.y != aSize.x && aSize.y != aSize.z;
		bool zExtended = aSize.z != aSize.x && aSize.z != aSize.y;
		auto GetWeights = [](bool aIsTo) {
			return aIsTo ? glm::vec4{ 0, 1, 0, 0 } : glm::vec4{ 1, 0, 0, 0 };
		};
		constexpr glm::vec3 kNoNormal = glm::vec3(0);
		// front
		verts.push_back({
			aCenter + glm::vec3(-aSize.x, -aSize.y, aSize.z),
			kNoNormal,
			{ 0, 0 },
			{ aBone, aBone + 1u, 0, 0 },
			GetWeights(zExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(aSize.x, -aSize.y, aSize.z), 
			kNoNormal,
			{1, 0}, 
			{ aBone, aBone + 1u, 0, 0 }, 
			GetWeights(xExtended || zExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(aSize.x, aSize.y, aSize.z), 
			kNoNormal,
			{1, 1}, 
			{ aBone, aBone + 1u, 0, 0 },
			GetWeights(xExtended || yExtended || zExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(-aSize.x, aSize.y, aSize.z), 
			kNoNormal,
			{0, 1}, 
			{ aBone, aBone + 1u, 0, 0 },
			GetWeights(yExtended || zExtended)
		});

		// back
		verts.push_back({ 
			aCenter + glm::vec3(-aSize.x, -aSize.y, -aSize.z), 
			kNoNormal,
			{1, 1}, 
			{ aBone, aBone + 1u, 0, 0 },
			GetWeights(false)
		});
		verts.push_back({ 
			aCenter + glm::vec3(aSize.x, -aSize.y, -aSize.z), 
			kNoNormal,
			{0, 1}, 
			{ aBone, aBone + 1u, 0, 0 },
			GetWeights(xExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(aSize.x, aSize.y, -aSize.z), 
			kNoNormal,
			{0, 0}, 
			{ aBone, aBone + 1u, 0, 0 },
			GetWeights(xExtended || yExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(-aSize.x, aSize.y, -aSize.z),
			kNoNormal,
			{1, 0}, 
			{ aBone, aBone + 1u, 0, 0 },
			GetWeights(yExtended)
		});
		
		indices.reserve(indices.size() + 36);
		indices.push_back(indexStart + 0);
		indices.push_back(indexStart + 1);
		indices.push_back(indexStart + 2);
		indices.push_back(indexStart + 2);
		indices.push_back(indexStart + 3);
		indices.push_back(indexStart + 0);

		indices.push_back(indexStart + 1);
		indices.push_back(indexStart + 5);
		indices.push_back(indexStart + 6);
		indices.push_back(indexStart + 6);
		indices.push_back(indexStart + 2);
		indices.push_back(indexStart + 1);

		indices.push_back(indexStart + 7);
		indices.push_back(indexStart + 6);
		indices.push_back(indexStart + 5);
		indices.push_back(indexStart + 5);
		indices.push_back(indexStart + 4);
		indices.push_back(indexStart + 7);

		indices.push_back(indexStart + 4);
		indices.push_back(indexStart + 0);
		indices.push_back(indexStart + 3);
		indices.push_back(indexStart + 3);
		indices.push_back(indexStart + 7);
		indices.push_back(indexStart + 4);

		indices.push_back(indexStart + 4);
		indices.push_back(indexStart + 5);
		indices.push_back(indexStart + 1);
		indices.push_back(indexStart + 1);
		indices.push_back(indexStart + 0);
		indices.push_back(indexStart + 4);

		indices.push_back(indexStart + 3);
		indices.push_back(indexStart + 2);
		indices.push_back(indexStart + 6);
		indices.push_back(indexStart + 6);
		indices.push_back(indexStart + 7);
		indices.push_back(indexStart + 3);
	};

	// Generate a cube with an arm for every bone (except root)
	constexpr float kMeshSize = 1.f / 4.f;
	for (Skeleton::BoneIndex boneInd = 1; boneInd < aSkeleton.GetBoneCount(); boneInd++)
	{
		// cube
		const Transform& bone = aSkeleton.GetBoneWorldTransform(boneInd);
		appendAACuboid(bone.GetPos(), glm::vec3(kMeshSize), boneInd);

		Skeleton::BoneIndex parentBoneInd = aSkeleton.GetParentIndex(boneInd);
		if (parentBoneInd != 0)
		{
			// arm
			const Transform& parentBone = aSkeleton.GetBoneWorldTransform(parentBoneInd);
			glm::vec3 delta = (bone.GetPos() - parentBone.GetPos()) / 2.f;
			glm::vec3 center = parentBone.GetPos() + delta;
			if (!glm::epsilonEqual(delta.x, 0.f, 0.0001f))
			{
				delta.y = kMeshSize / 4;
				delta.z = kMeshSize / 4;
			}
			else if (!glm::epsilonEqual(delta.y, 0.f, 0.0001f))
			{
				delta.x = kMeshSize / 4;
				delta.z = kMeshSize / 4;
			}
			else if (!glm::epsilonEqual(delta.z, 0.f, 0.0001f))
			{
				delta.x = kMeshSize / 4;
				delta.y = kMeshSize / 4;
			}
			else
			{
				ASSERT(false);
			}
			appendAACuboid(center, delta, parentBoneInd);
		}
	}

	Model::VertStorage<SkinnedVertex>* storage = new Model::VertStorage<SkinnedVertex>(verts.size());
	Model* model = new Model(Model::PrimitiveType::Triangles, storage, true);

	Model::UploadDescriptor<SkinnedVertex> uploadDesc;
	uploadDesc.myVertices = verts.data();
	uploadDesc.myVertCount = verts.size();
	uploadDesc.myIndices = indices.data();
	uploadDesc.myIndCount = indices.size();
	uploadDesc.myNextDesc = nullptr;
	uploadDesc.myVertsOwned = false;
	uploadDesc.myIndOwned = false;
	model->Update(uploadDesc);

	glm::vec3 min(std::numeric_limits<float>::max());
	glm::vec3 max(std::numeric_limits<float>::min());
	for (const SkinnedVertex& vert : verts)
	{
		min = glm::min(min, vert.myPos);
		max = glm::max(max, vert.myPos);
	}
	model->SetAABB(min, max);

	return model;
}