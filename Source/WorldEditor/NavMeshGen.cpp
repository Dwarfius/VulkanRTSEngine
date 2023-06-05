#include "Precomp.h"
#include "NavMeshGen.h"

#include <Engine/Game.h>
#include <Engine/World.h>
#include <Engine/Components/VisualComponent.h>

#include <Graphics/Resources/Model.h>
#include <Graphics/Utils.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Debug/DebugDrawer.h>

void NavMeshGen::Generate(const Input& anInput, const Settings& aSettings, Game& aGame)
{
	ASSERT_STR(static_cast<uint64_t>(anInput.myMax.x - anInput.myMin.x) 
		/ kVoxelSize < std::numeric_limits<uint32_t>::max(),
		"u32 is not enough to fit the grid!");
	ASSERT(static_cast<uint64_t>(anInput.myMax.y - anInput.myMin.y) 
		/ kVoxelHeight < std::numeric_limits<uint32_t>::max());
	ASSERT(static_cast<uint64_t>(anInput.myMax.z - anInput.myMin.z) 
		/ kVoxelSize < std::numeric_limits<uint32_t>::max());

	myInput = anInput;
	mySettings = aSettings;

	GatherTriangles(aGame.GetAssetTracker());
}

void NavMeshGen::DebugDraw(DebugDrawer& aDrawer) const
{
	if (mySettings.myDrawValidTriangleChecks)
	{
		myTile.DrawValidTriangleChecks(aDrawer);
	}

	if (mySettings.myDrawGeneratedSpans)
	{
		myTile.DrawVoxelSpans(aDrawer, myInput);
	}
}

void NavMeshGen::VoxelColumn::AddBoth(uint32_t aHeight)
{
	// try to extend the span
	if (!mySpans.empty())
	{
		VoxelSpan& span = mySpans.back();
		if (span.myMaxY == aHeight)
		{
			span.myMaxY++;
			return;
		}
	}

	mySpans.push_back(VoxelSpan{ aHeight, aHeight + 1, 0 });
}

void NavMeshGen::Tile::Insert(glm::vec3 aV1, glm::vec3 aV2, glm::vec3 aV3, const Input& aInput)
{
	auto Quantize = [](glm::vec3 aVec) {
		return glm::u32vec3{ 
			aVec.x / kVoxelSize,
			aVec.y / kVoxelHeight,
			aVec.z / kVoxelSize
		};
	};
	// quantized in voxel grid
	const glm::u32vec3 v1 = Quantize(aV1 - aInput.myMin);
	const glm::u32vec3 v2 = Quantize(aV2 - aInput.myMin);
	const glm::u32vec3 v3 = Quantize(aV3 - aInput.myMin);

	const glm::u32vec3 minBV = glm::min(v1, glm::min(v2, v3));
	const glm::u32vec3 maxBV = glm::max(v1, glm::max(v2, v3));

	// TODO: optimize as this can cover a lot of empty cells
	// with large triangles. Instead of using BV of triangle,
	// just "rasterize" the triangle

	// Note on -1 for Y: because of floating point inacuracy and rounding, I've
	// seen cases where it picked voxels outside of triangle, so it never
	// passed the intersection check. I've tried fudging the numbers to avoid
	// rounding inaccuracy, but it wasn't applicable to all cases. So I'm
	// adding the extra height voxel to check - it's inefficient, but stable
	const uint32_t minY = minBV.y > 1 ? minBV.y - 1 : minBV.y;
	for (uint32_t z = minBV.z; z <= maxBV.z; z++)
	{
		for (uint32_t x = minBV.x; x <= maxBV.x; x++)
		{
			for (uint32_t y = minY; y <= maxBV.y; y++)
			{
				const glm::vec3 voxelMin{
					x * kVoxelSize,
					y * kVoxelHeight,
					z * kVoxelSize
				};
				const glm::vec3 voxelMax{
					(x + 1) * kVoxelSize,
					(y + 1) * kVoxelHeight,
					(z + 1) * kVoxelSize
				};

				const Utils::AABB voxelAABB{ 
					aInput.myMin + voxelMin, 
					aInput.myMin + voxelMax 
				};
				
				const bool intersects = Utils::Intersects(aV1, aV2, aV3, voxelAABB);
				if (intersects)
				{
					// TODO: track top and bottom!
					VoxelColumn& column = myVoxelGrid.at(z * mySize.x + x);
					column.AddBoth(y);
				}
			}
		}
	}
}

void NavMeshGen::Tile::DrawValidTriangleChecks(DebugDrawer& aDrawer) const
{
	for (const Line& line : myDebugTriangles)
	{
		aDrawer.AddLine(line.myStart, line.myEnd, line.myColor);
	}
}

void NavMeshGen::Tile::DrawVoxelSpans(DebugDrawer& aDrawer, const Input& aInput) const
{
	for (uint32_t z = 0; z < mySize.y; z++)
	{
		for (uint32_t x = 0; x < mySize.x; x++)
		{
			const VoxelColumn& column = myVoxelGrid[z * mySize.x + x];
			for (const VoxelSpan& span : column.mySpans)
			{
				const glm::vec3 min{
					x * kVoxelSize,
					span.myMinY * kVoxelHeight,
					z * kVoxelSize
				};
				const glm::vec3 max{
					(x + 1) * kVoxelSize,
					span.myMaxY * kVoxelHeight,
					(z + 1) * kVoxelSize
				};
				aDrawer.AddAABB(
					aInput.myMin + min,
					aInput.myMin + max,
					{ 0, 1, 0 }
				);
			}
		}
	}
}

void NavMeshGen::GatherTriangles(AssetTracker& anAssetTracker)
{
	const glm::vec3 bvWidth = myInput.myMax - myInput.myMin;
	myTile.mySize = glm::u32vec2{ 
		glm::ceil(bvWidth.x / kVoxelSize), 
		glm::ceil(bvWidth.z / kVoxelSize) 
	};
	myTile.myMinHeight = myInput.myMin.y;
	myTile.myMaxHeight = myInput.myMax.y;
	myTile.myVoxelGrid.resize((myTile.mySize.x + 1) * (myTile.mySize.y + 1));

	myInput.myWorld->Access([&, this](const std::vector<Handle<GameObject>>& aGOs) {
		const float maxSlopeCos = glm::cos(glm::radians(mySettings.myMaxSlope));
		for (const Handle<GameObject>& go : aGOs)
		{
			const VisualComponent* visual = go->GetComponent<VisualComponent>();
			if (!visual)
			{
				continue;
			}

			const Resource::Id modelRes = visual->GetModelId();
			if (modelRes == Resource::InvalidId)
			{
				continue;
			}

			Handle<Model> model = anAssetTracker.Get<Model>(modelRes);
			ASSERT_STR(model->GetState() == Resource::State::Ready, 
				"Not ready to generate navmesh!");

			const Transform& transf = go->GetWorldTransform();
			const glm::mat4& transfMat = transf.GetMatrix();

			// TODO: BV check!

			if (mySettings.myDrawValidTriangleChecks)
			{
				myTile.myDebugTriangles.reserve(
					myTile.myDebugTriangles.size() + model->GetIndexCount() / 3
				);
			}

			const VertexDescriptor& desc = model->GetVertexDescriptor();
			// TODO: need to find to generalize access to Pos
			if (desc == Vertex::GetDescriptor())
			{
				const Vertex* vertices = static_cast<const Vertex*>(model->GetVertices());
				const Model::IndexType* indices = model->GetIndices();
				for (Model::IndexType i = 0; i < model->GetIndexCount(); i += 3)
				{
					const glm::vec4 v1LS{ vertices[indices[i]].myPos, 1.f };
					const glm::vec4 v2LS{ vertices[indices[i + 1]].myPos, 1.f };
					const glm::vec4 v3LS{ vertices[indices[i + 2]].myPos, 1.f };

					const glm::vec3 v1 = transfMat * v1LS;
					const glm::vec3 v2 = transfMat * v2LS;
					const glm::vec3 v3 = transfMat * v3LS;

					// angle against horizontal plane
					const glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
					const float slopeCos = glm::dot(normal, { 0, 1, 0 });
					const bool validTriangle = slopeCos >= maxSlopeCos;

					if (mySettings.myDrawValidTriangleChecks)
					{
						const glm::vec3 center = (v1 + v2 + v3) / 3.f;
						const glm::vec3 color = validTriangle ? glm::vec3{ 0, 1, 0 } : glm::vec3{ 1, 0, 0 };
						myTile.myDebugTriangles.push_back({ center, center + normal / 4.f, color });
					}

					if (!validTriangle)
					{
						continue;
					}

					myTile.Insert(v1, v2, v3, myInput);
				}
			}
		}
	});
}