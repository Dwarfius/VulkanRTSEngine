#include "Precomp.h"
#include "AnimationTest.h"

#include "../Game.h"
#include "../VisualObject.h"
#include <Core/Resources/AssetTracker.h>
#include <Graphics/Resources/Model.h>
#include "../Animation/AnimationSystem.h"
#include "../Animation/AnimationClip.h"
#include "../Resources/GLTFImporter.h"

AnimationTest::AnimationTest(Game& aGame)
	: myGame(aGame)
{
	myGO = myGame.Instantiate();

	PoolPtr<Skeleton> testSkeleton = aGame.GetAnimationSystem().AllocateSkeleton(4);
	Skeleton* skeleton = testSkeleton.Get();

	skeleton->AddBone(Skeleton::kInvalidIndex, {});
	skeleton->AddBone(0, { glm::vec3(0, 1, 0), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->AddBone(1, { glm::vec3(0, 0, 1), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->AddBone(2, { glm::vec3(0, 1, 0), glm::vec3(0.f), glm::vec3(1.f) });
	skeleton->Update();

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
	
	myGO->SetSkeleton(std::move(testSkeleton));
	myGO->SetAnimController(std::move(testController));

	Handle<Pipeline> skinnedPipeline = myGame.GetAssetTracker().GetOrCreate<Pipeline>("skinned.ppl");
	Handle<Texture> wireframeTexture = myGame.GetAssetTracker().GetOrCreate<Texture>("wireframe.png");
	VisualObject* vo = new VisualObject(*myGO);
	vo->SetPipeline(skinnedPipeline);
	vo->SetTexture(wireframeTexture);
	vo->SetModel(GenerateModel(*skeleton));
	myGO->SetVisualObject(vo);
}

AnimationTest::~AnimationTest()
{
	myGame.RemoveGameObject(myGO);
}

void AnimationTest::Update(float aDeltaTime)
{
	DebugDrawer& drawer = myGame.GetDebugDrawer();
	myGO->GetSkeleton().Get()->DebugDraw(drawer, { glm::vec3(0), glm::vec3(0), glm::vec3(1) });
}

struct AnimationVert
{
	glm::vec3 myPos;
	glm::vec2 myUv;
	uint32_t myBoneIndices[2];
	glm::vec2 myBoneWeights;

	static constexpr VertexDescriptor GetDescriptor()
	{
		using ThisType = AnimationVert; // for copy-paste convenience
		return {
			sizeof(ThisType),
			4,
			{
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myPos) },
				{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myUv) },
				{ VertexDescriptor::MemberType::U32, 2, offsetof(ThisType, myBoneIndices) },
				{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myBoneWeights) },
			}
		};
	}
};

namespace std
{
	template<> struct hash<AnimationVert>
	{
		std::size_t operator()(const AnimationVert& v) const
		{
			return ((hash<glm::vec3>()(v.myPos) ^
				(hash<glm::vec2>()(v.myUv) << 1)) >> 1) ^
				(hash<uint16_t>()(v.myBoneIndices[0]) << 2) ^
				(hash<uint16_t>()(v.myBoneIndices[1]) >> 2) ^
				(hash<glm::vec2>()(v.myBoneWeights) >> 2);
		}
	};
}

Handle<Model> AnimationTest::GenerateModel(const Skeleton& aSkeleton)
{
	// for each skeleton bone - 1 we need a cube and an arm (rectangular cuboid)
	std::vector<AnimationVert> verts;
	std::vector<IModel::IndexType> indices;
	auto appendAACuboid = [&](glm::vec3 aCenter, glm::vec3 aSize, uint16_t aBone)
	{
		IModel::IndexType indexStart = static_cast<IModel::IndexType>(verts.size());
		verts.reserve(verts.size() + 8);

		bool xExtended = aSize.x != aSize.y && aSize.x != aSize.z;
		bool yExtended = aSize.y != aSize.x && aSize.y != aSize.z;
		bool zExtended = aSize.z != aSize.x && aSize.z != aSize.y;
		auto GetWeights = [](bool aIsTo) {
			return aIsTo ? glm::vec2{0, 1} : glm::vec2{ 1, 0 };
		};

		// front
		verts.push_back({
			aCenter + glm::vec3(-aSize.x, -aSize.y, aSize.z),
			{0, 0},
			{aBone, aBone + 1u},
			GetWeights(zExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(aSize.x, -aSize.y, aSize.z), 
			{1, 0}, 
			{aBone, aBone + 1u}, 
			GetWeights(xExtended || zExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(aSize.x, aSize.y, aSize.z), 
			{1, 1}, 
			{aBone, aBone + 1u}, 
			GetWeights(xExtended || yExtended || zExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(-aSize.x, aSize.y, aSize.z), 
			{0, 1}, 
			{aBone, aBone + 1u}, 
			GetWeights(yExtended || zExtended)
		});

		// back
		verts.push_back({ 
			aCenter + glm::vec3(-aSize.x, -aSize.y, -aSize.z), 
			{1, 1}, 
			{aBone, aBone + 1u}, 
			{1, 0} 
		});
		verts.push_back({ 
			aCenter + glm::vec3(aSize.x, -aSize.y, -aSize.z), 
			{0, 1}, 
			{aBone, aBone + 1u}, 
			GetWeights(xExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(aSize.x, aSize.y, -aSize.z), 
			{0, 0}, 
			{aBone, aBone + 1u}, 
			GetWeights(xExtended || yExtended)
		});
		verts.push_back({ 
			aCenter + glm::vec3(-aSize.x, aSize.y, -aSize.z),
			{1, 0}, 
			{aBone, aBone + 1u}, 
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

	Model::VertStorage<AnimationVert>* storage = new Model::VertStorage<AnimationVert>(verts.size());
	Model* model = new Model(PrimitiveType::Triangles, storage, true);

	Model::UploadDescriptor<AnimationVert> uploadDesc;
	uploadDesc.myVertices = verts.data();
	uploadDesc.myVertCount = verts.size();
	uploadDesc.myIndices = indices.data();
	uploadDesc.myIndCount = indices.size();
	uploadDesc.myNextDesc = nullptr;
	uploadDesc.myVertsOwned = false;
	uploadDesc.myIndOwned = false;
	model->Update(uploadDesc);

	return model;
}