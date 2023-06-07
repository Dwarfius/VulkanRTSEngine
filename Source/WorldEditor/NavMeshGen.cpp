#include "Precomp.h"
#include "NavMeshGen.h"

#include <Engine/Game.h>
#include <Engine/World.h>
#include <Engine/Components/VisualComponent.h>

#include <Graphics/Resources/Model.h>
#include <Graphics/Utils.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Debug/DebugDrawer.h>
#include <Core/Profiler.h>

void NavMeshGen::Generate(const Input& anInput, const Settings& aSettings, Game& aGame)
{
	Profiler::ScopedMark scope("NavMesh::Generate");
	myInput = anInput;
	mySettings = aSettings;

	CreateTiles();
	GatherTriangles(aGame.GetAssetTracker());
}

void NavMeshGen::DebugDraw(DebugDrawer& aDrawer) const
{
	if (mySettings.myDrawGenAABB)
	{
		aDrawer.AddAABB(myInput.myMin, myInput.myMax, { 1, 1, 1 });

		for (const Tile& tile : myTiles)
		{
			const glm::vec3 aabbMax{
				tile.myAABBMin.x + tile.mySize.x * kVoxelSize,
				tile.myAABBMin.y + tile.mySize.y * kVoxelHeight,
				tile.myAABBMin.z + tile.mySize.z * kVoxelSize,
			};
			aDrawer.AddAABB(tile.myAABBMin, aabbMax, { 0, 1, 0 });
		}
	}

	if (mySettings.myDrawValidTriangleChecks)
	{
		for (const Tile& tile : myTiles)
		{
			tile.DrawValidTriangleChecks(aDrawer);
		}
	}

	if (mySettings.myDrawGeneratedSpans)
	{
		for (const Tile& tile : myTiles)
		{
			tile.DrawVoxelSpans(aDrawer);
		}
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

void NavMeshGen::Tile::Insert(glm::vec3 aV1, glm::vec3 aV2, glm::vec3 aV3)
{
	auto Quantize = [](glm::vec3 aVec) {
		return glm::i32vec3{ 
			aVec.x / kVoxelSize,
			aVec.y / kVoxelHeight,
			aVec.z / kVoxelSize
		};
	};
	// quantized in voxel grid
	const glm::i32vec3 v1 = Quantize(aV1 - myAABBMin);
	const glm::i32vec3 v2 = Quantize(aV2 - myAABBMin);
	const glm::i32vec3 v3 = Quantize(aV3 - myAABBMin);

	const glm::i32vec3 minBV = glm::min(v1, glm::min(v2, v3));
	const glm::i32vec3 maxBV = glm::max(v1, glm::max(v2, v3));

	// Entire triangle might lie outside of our tile
	if (!Utils::Intersects(Utils::AABB{ minBV, maxBV }, Utils::AABB{ {0, 0, 0}, mySize }))
		return;

	// TODO: optimize as this can cover a lot of empty cells
	// with large triangles. Instead of using BV of triangle,
	// just "rasterize" the triangle

	// Vertices can lie outside of the voxel grid
	// so clamp the bounding voxels
	const uint32_t minX = static_cast<uint32_t>(glm::max(minBV.x, 0));
	const uint32_t maxX = static_cast<uint32_t>(
		glm::min(maxBV.x, static_cast<int32_t>(mySize.x))
	);

	// Note on -1 for Y: because of floating point inacuracy and rounding, I've
	// seen cases where it picked voxels outside of triangle, so it never
	// passed the intersection check. I've tried fudging the numbers to avoid
	// rounding inaccuracy, but it wasn't applicable to all cases. So I'm
	// adding the extra height voxel to check - it's inefficient, but stable
	const uint32_t minY = static_cast<uint32_t>(
		minBV.y > 1 ? minBV.y - 1 : glm::max(minBV.y, 0)
	);
	const uint32_t maxY = static_cast<uint32_t>(
		glm::min(maxBV.y, static_cast<int32_t>(mySize.y))
	);

	const uint32_t minZ = static_cast<uint32_t>(glm::max(minBV.z, 0));
	const uint32_t maxZ = static_cast<uint32_t>(
		glm::min(maxBV.z, static_cast<int32_t>(mySize.z))
	);
	for (uint32_t z = minZ; z < maxZ; z++)
	{
		for (uint32_t x = minX; x < maxX; x++)
		{
			for (uint32_t y = minY; y <= maxY; y++)
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
					myAABBMin + voxelMin,
					myAABBMin + voxelMax
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

void NavMeshGen::Tile::DrawVoxelSpans(DebugDrawer& aDrawer) const
{
	for (uint32_t z = 0; z < mySize.z; z++)
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
					myAABBMin + min,
					myAABBMin + max,
					{ 0, 1, 0 }
				);
			}
		}
	}
}

void NavMeshGen::CreateTiles()
{
	constexpr float kTileSize = kVoxelSize * kVoxelsPerTile;
	const glm::vec3 bvSize = myInput.myMax - myInput.myMin;
	const glm::u32vec2 tileCount{
		glm::ceil(bvSize.x / kTileSize),
		glm::ceil(bvSize.z / kTileSize)
	};
	myTiles.resize(tileCount.x * tileCount.y);

	// Round to kVoxelSize
	const float maxX = glm::ceil(myInput.myMax.x / kVoxelSize) * kVoxelSize;
	const float maxZ = glm::ceil(myInput.myMax.z / kVoxelSize) * kVoxelSize;
	for (uint32_t y = 0; y < tileCount.y; y++)
	{
		for (uint32_t x = 0; x < tileCount.x; x++)
		{
			const glm::vec2 bvMin{ 
				myInput.myMin.x + x * kTileSize, 
				myInput.myMin.z + y * kTileSize 
			};
			const glm::vec2 bvMax{
				glm::min(bvMin.x + kTileSize, maxX),
				glm::min(bvMin.y + kTileSize, maxZ)
			};

			Tile& tile = myTiles[y * tileCount.x + x];
			tile.mySize = glm::u32vec3{
				glm::ceil((bvMax.x - bvMin.x) / kVoxelSize),
				glm::ceil(bvSize.y / kVoxelHeight),
				glm::ceil((bvMax.y - bvMin.y) / kVoxelSize)
			};
			tile.myAABBMin = glm::vec3{
				bvMin.x,
				myInput.myMin.y,
				bvMin.y
			};
			tile.myMinHeight = myInput.myMin.y;
			tile.myMaxHeight = myInput.myMax.y;
			tile.myVoxelGrid.resize(tile.mySize.x * tile.mySize.z);
		}
	}
}

void NavMeshGen::GatherTriangles(AssetTracker& anAssetTracker)
{
	const float maxSlopeCos = glm::cos(glm::radians(mySettings.myMaxSlope));
	auto ProcessForTile = [&](Tile& aTile, const std::vector<Handle<GameObject>>& aGOs) {
		Profiler::ScopedMark scope("NavMesh::GatherTriangles::ProcessTile");

		const glm::vec3 tileMax{
			aTile.myAABBMin.x + aTile.mySize.x * kVoxelSize,
			aTile.myAABBMin.y + aTile.mySize.y * kVoxelHeight,
			aTile.myAABBMin.z + aTile.mySize.z * kVoxelSize
		};
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

			// AABB early model rejection
			{
				const Utils::AABB modelAABB{
					model->GetAABBMin(),
					model->GetAABBMax()
				};
				const Utils::AABB transfAABB = modelAABB.Transform(transf);
				if (!Utils::Intersects(transfAABB, { aTile.myAABBMin, tileMax }))
				{
					continue;
				}
			}

			if (mySettings.myDrawValidTriangleChecks)
			{
				aTile.myDebugTriangles.reserve(
					aTile.myDebugTriangles.size() + model->GetIndexCount() / 3
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
						aTile.myDebugTriangles.push_back({ center, center + normal / 4.f, color });
					}

					if (!validTriangle)
					{
						continue;
					}

					aTile.Insert(v1, v2, v3);
				}
			}
		}
	};

	myInput.myWorld->Access([&](const std::vector<Handle<GameObject>>& aGOs) {
		tbb::parallel_for(tbb::blocked_range<size_t>(0, myTiles.size()),
			[&](tbb::blocked_range<size_t> aRange) 
		{
			for (size_t i = aRange.begin(); i < aRange.end(); i++)
			{
				Tile& tile = myTiles[i];
				ProcessForTile(tile, aGOs);
			}
		});
	});
}