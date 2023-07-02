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
	Profiler::GetInstance().CaptureCurrentFrame();
	Profiler::ScopedMark scope("NavMesh::Generate");
	myInput = anInput;
	mySettings = aSettings;

	CreateTiles();
	GatherTriangles(aGame.GetAssetTracker());
	SegmentTiles();
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

	if (mySettings.myDrawRegions)
	{
		for (const Region& region : myRegions)
		{
			region.Draw(aDrawer);
		}
	}

	if (mySettings.myDrawCornerPoints)
	{
		for (const Region& region : myRegions)
		{
			region.DrawCornerPoints(aDrawer);
		}
	}
}

bool NavMeshGen::IsVertexCorner(uint8_t aVert, uint8_t aNeighborsSet)
{
	// Note: kNeighborEncTable in NavMeshGen::SegmentTiles()
	constexpr static uint8_t kCornerMasks[]{
		11, 22, 104, 208
	};
	// If the vertex has any of these neighbors 
	// then it's not a corner
	constexpr static uint8_t kNeighbors[][3]{
		{11, 8, 2},
		{22, 2, 16},
		{104, 8, 64},
		{208, 16,64}
	};
	const uint8_t neighbors = aNeighborsSet & kCornerMasks[aVert];
	for (uint8_t i = 0; i < 3; i++)
	{
		if (neighbors == kNeighbors[aVert][i])
		{
			return false;
		}
	}
	return true;
}

bool NavMeshGen::IsVertexSlashPoint(uint8_t aVert, uint8_t aNeighborsSet)
{
	// Note: kNeighborEncTable in NavMeshGen::SegmentTiles()
	constexpr static uint8_t kCornerMasks[]{
		11, 22, 104, 208
	};
	// If the vertex has any of these neighbors 
	// then it's not a corner
	constexpr static uint8_t kNeighbors[]{
		1, 4, 32, 128
	};
	const uint8_t neighbors = aNeighborsSet & kCornerMasks[aVert];
	return neighbors == kNeighbors[aVert] && IsVertexCorner(aVert, aNeighborsSet);
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

	mySpans.push_back(VoxelSpan{ aHeight, aHeight + 1, VoxelSpan::kInvalidRegion, 0 });
}

void NavMeshGen::VoxelColumn::Merge()
{
	if (mySpans.empty())
	{
		return;
	}

	std::sort(mySpans.begin(), mySpans.end(),
		[](const VoxelSpan& aLeft, const VoxelSpan& aRight) 
		{
			return aLeft.myMinY < aRight.myMinY;
		}
	);

	for (uint32_t i = 0; i < mySpans.size() - 1; i++)
	{
		VoxelSpan& span = mySpans[i];
		VoxelSpan& nextSpan = mySpans[i + 1];
		
		if (span.myMaxY >= nextSpan.myMinY && span.myMinY <= nextSpan.myMaxY)
		{
			nextSpan.myMinY = glm::min(span.myMinY, nextSpan.myMinY);
			nextSpan.myMaxY = glm::max(span.myMaxY, nextSpan.myMaxY);

			span.myMinY = span.myMaxY; // reset a span
		}
	}

	std::erase_if(mySpans, 
		[](const VoxelSpan& aSpan) { return aSpan.myMinY == aSpan.myMaxY; }
	);
}

void NavMeshGen::Tile::Insert(glm::vec3 aV1, glm::vec3 aV2, glm::vec3 aV3)
{
	const glm::vec3 minBVWS = glm::min(aV1, glm::min(aV2, aV3));
	const glm::vec3 maxBVWS = glm::max(aV1, glm::max(aV2, aV3));

	// Entire triangle might lie outside of our tile
	const glm::vec3 aabbMax{
		myAABBMin.x + mySize.x * kVoxelSize,
		myAABBMin.y + mySize.y * kVoxelHeight,
		myAABBMin.z + mySize.z * kVoxelSize,
	};
	if (!Utils::Intersects(
		Utils::AABB{ minBVWS, maxBVWS },
		Utils::AABB{ myAABBMin, aabbMax }))
	{
		return;
	}

	// Quantize in voxel grid
	const glm::vec3 minBV{
		glm::round((minBVWS.x - myAABBMin.x) / kVoxelSize),
		glm::round((minBVWS.y - myAABBMin.y) / kVoxelHeight),
		glm::round((minBVWS.z - myAABBMin.z) / kVoxelSize)
	};

	const glm::vec3 maxBV{
		glm::round((maxBVWS.x - myAABBMin.x) / kVoxelSize),
		glm::round((maxBVWS.y - myAABBMin.y) / kVoxelHeight),
		glm::round((maxBVWS.z - myAABBMin.z) / kVoxelSize)
	};

	// TODO: optimize as this can cover a lot of empty cells
	// with large triangles. Instead of using BV of triangle,
	// just "rasterize" the triangle

	// Vertices can lie outside of the voxel grid
	// so clamp the bounding voxels
	const uint32_t minX = static_cast<uint32_t>(glm::max(minBV.x, 0.f));
	const uint32_t maxX = static_cast<uint32_t>(
		glm::min(maxBV.x, static_cast<float>(mySize.x))
	);

	const uint32_t minY = static_cast<uint32_t>(glm::max(minBV.y, 0.f));
	const uint32_t maxY = static_cast<uint32_t>(
		glm::min(maxBV.y, static_cast<float>(mySize.y))
	);

	const uint32_t minZ = static_cast<uint32_t>(glm::max(minBV.z, 0.f));
	const uint32_t maxZ = static_cast<uint32_t>(
		glm::min(maxBV.z, static_cast<float>(mySize.z))
	);

	// It's possible to lose precision as we're translating from voxel space
	// (above min/max coords) to world space, resulting in failing triangle-AABB
	// tests. This happens because we're comparing triangle's WS coords that are
	// precise, to coords that have made a round trip transformation 
	// (WS -> VS -> WS) - so we avoid it, instead bringing it all to VS.
	// Translate into tile space to maintain precision first
	const glm::vec3 v1TS = aV1 - myAABBMin;
	const glm::vec3 v2TS = aV2 - myAABBMin;
	const glm::vec3 v3TS = aV3 - myAABBMin;

	constexpr Utils::AABB voxelAABB{
		{ 0, 0, 0 },
		{ kVoxelSize, kVoxelHeight, kVoxelSize }
	};
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
				// Translate to voxel space to maintain precision
				const glm::vec3 v1VS = v1TS - voxelMin;
				const glm::vec3 v2VS = v2TS - voxelMin;
				const glm::vec3 v3VS = v3TS - voxelMin;

				const bool intersects = Utils::Intersects(v1VS, v2VS, v3VS, voxelAABB);
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

void NavMeshGen::Tile::MergeColumns()
{
	Profiler::ScopedMark scope("NavMeshGen::Tile::MergeColumns");
	for (VoxelColumn& column : myVoxelGrid)
	{
		column.Merge();
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
	// Round to kTileSize
	constexpr float kTileSize = kVoxelSize * kVoxelsPerTile;
	const float minX = glm::floor(myInput.myMin.x / kTileSize) * kTileSize;
	const float minZ = glm::floor(myInput.myMin.z / kTileSize) * kTileSize;
	const float maxX = glm::ceil(myInput.myMax.x / kTileSize) * kTileSize;
	const float maxZ = glm::ceil(myInput.myMax.z / kTileSize) * kTileSize;
	const float height = glm::ceil(myInput.myMax.y - myInput.myMin.y) / kVoxelHeight;
	
	const glm::u32vec2 tileCount{
		glm::ceil((maxX - minX) / kTileSize),
		glm::ceil((maxZ - minZ) / kTileSize)
	};
	myTiles.resize(tileCount.x * tileCount.y);

	for (uint32_t y = 0; y < tileCount.y; y++)
	{
		for (uint32_t x = 0; x < tileCount.x; x++)
		{
			const glm::vec2 bvMin{ 
				glm::max(minX + x * kTileSize, myInput.myMin.x),
				glm::max(minZ + y * kTileSize, myInput.myMin.z)
			};
			const glm::vec2 bvMax{
				glm::min(minX + (x + 1) * kTileSize, myInput.myMax.x),
				glm::min(minZ + (y + 1) * kTileSize, myInput.myMax.z)
			};

			Tile& tile = myTiles[y * tileCount.x + x];
			tile.mySize = glm::u32vec3{
				glm::ceil((bvMax.x - bvMin.x) / kVoxelSize),
				height,
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
		Profiler::ScopedMark scope("NavMeshGen::GatherTriangles::ProcessTile");

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

		aTile.MergeColumns();
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

void NavMeshGen::Region::Draw(DebugDrawer& aDrawer) const
{
	constexpr float kComp = 1.f;
	constexpr glm::vec3 kColorTable[]{
		{ 0, kComp, kComp },
		{ kComp, 0, kComp },
		{ kComp, kComp, 0 },
		{ 0.25f, kComp, kComp },
		{ kComp, 0.25f, kComp },
		{ kComp, kComp, 0.25f },
		{ 0.75f, kComp, kComp },
		{ kComp, 0.75f, kComp },
		{ kComp, kComp, 0.75f }
	};
	const glm::vec3 color = kColorTable[myRegionId % std::size(kColorTable)];
	for (const SpanPos& span : mySpans)
	{
		const glm::vec3 p1 = myTileAABBMin + glm::vec3{
			span.myPos.x * kVoxelSize, 
			myHeight * kVoxelHeight, 
			span.myPos.y * kVoxelSize
		};
		const glm::vec3 p2 = p1 + glm::vec3{ kVoxelSize, 0, 0 };
		const glm::vec3 p3 = p1 + glm::vec3{ 0, 0, kVoxelSize };
		const glm::vec3 p4 = p2 + glm::vec3{ 0, 0, kVoxelSize };

		aDrawer.AddLine(p1, p2, color);
		aDrawer.AddLine(p2, p4, color);
		aDrawer.AddLine(p4, p3, color);
		aDrawer.AddLine(p3, p1, color);
	}
}

void NavMeshGen::Region::DrawCornerPoints(DebugDrawer& aDrawer) const
{
	constexpr static glm::vec3 kOffsets[]{
		{0, 0, kVoxelSize},
		{kVoxelSize, 0, kVoxelSize},
		{0, 0, 0},
		{kVoxelSize, 0, 0}
	};
	for (const SpanPos& spanPos : myCornerSpans)
	{
		const VoxelSpan& span = *spanPos.mySpan;
		const glm::vec3 spanOrigin = myTileAABBMin + glm::vec3{
			spanPos.myPos.x * kVoxelSize,
			span.myMaxY * kVoxelHeight,
			spanPos.myPos.y * kVoxelSize
		};
		for (uint8_t cornerInd = 0; cornerInd < 4; cornerInd++)
		{
			if (IsVertexSlashPoint(cornerInd, span.myNeighbors))
			{
				aDrawer.AddSphere(
					spanOrigin + kOffsets[cornerInd],
					kVoxelSize / 5.f,
					{ 1, 0, 0 }
				);
			}
			else if (IsVertexCorner(cornerInd, span.myNeighbors))
			{
				aDrawer.AddSphere(
					spanOrigin + kOffsets[cornerInd], 
					kVoxelSize / 5.f, 
					{ 0, 1, 0 }
				);
			}
		}
	}
}

void NavMeshGen::SegmentTiles()
{
	constexpr static uint8_t kNeighborEncTable[]{
		32, 64, 128,
		8, 0, 16,
		1, 2, 4
	};

	auto FindNeighbours = [](std::vector<glm::u32vec2>& aToCheck, Region& aRegion, Tile& aTile) {
		const glm::u32vec2 spanIndex = aToCheck.back();
		aToCheck.pop_back();

		const glm::i32vec2 neighborOrigin{
			spanIndex.x - 1,
			spanIndex.y - 1
		};
		const glm::u32vec2 min = glm::max(
			neighborOrigin,
			glm::i32vec2{ 0,0 }
		);
		const glm::u32vec2 max = glm::min(
			glm::u32vec2{ spanIndex.x + 1, spanIndex.y + 1 }, 
			glm::u32vec2{ aTile.mySize.x - 1, aTile.mySize.z - 1 }
		);
		
		uint8_t neighbors = 0;
		for (uint32_t z = min.y; z <= max.y; z++)
		{
			for (uint32_t x = min.x; x <= max.x; x++)
			{
				if (z == spanIndex.y && x == spanIndex.x)
				{
					continue;
				}

				VoxelColumn& neighborColumn = aTile.myVoxelGrid[z * aTile.mySize.x + x];
				for (VoxelSpan& span : neighborColumn.mySpans)
				{
					if (span.myMaxY < aRegion.myHeight)
					{
						continue;
					}
					else if (span.myMaxY > aRegion.myHeight)
					{
						break;
					}

					const glm::u32vec2 delta = glm::i32vec2{ x, z } - neighborOrigin;
					neighbors |= kNeighborEncTable[delta.y * 3 + delta.x];
					
					if (span.myRegionId != VoxelSpan::kInvalidRegion)
					{
						continue;
					}

					span.myRegionId = aRegion.myRegionId;
					aRegion.mySpans.push_back({ &span, {x, z} });
					aToCheck.push_back({ x, z });
					break;
				}
			}
		}
		
		// update the neighbors info
		bool foundCorner = false;
		for (uint8_t i = 0; i < 4; i++)
		{
			if (IsVertexCorner(i, neighbors))
			{
				foundCorner = true;
				break;
			}
		}

		if (foundCorner)
		{
			VoxelColumn& currColumn = aTile.myVoxelGrid[
				spanIndex.y * aTile.mySize.x + spanIndex.x
			];
			for (VoxelSpan& span : currColumn.mySpans)
			{
				if (span.myMaxY < aRegion.myHeight)
				{
					continue;
				}
				else if (span.myMaxY > aRegion.myHeight)
				{
					break;
				}

				aRegion.myCornerSpans.push_back({ &span, spanIndex });
				span.myNeighbors = neighbors;
				break;
			}
		}
	};

	for (Tile& tile : myTiles)
	{
		Profiler::ScopedMark scope("NavMeshGen::SegmentTiles::Tile");

		uint16_t regionCounter = 0;
		std::vector<glm::u32vec2> spansToCheck;
		// assume 1 span per voxel column
		spansToCheck.reserve(kVoxelsPerTile * kVoxelsPerTile);
		for (uint32_t z = 0; z < tile.mySize.z; z++)
		{
			for (uint32_t x = 0; x < tile.mySize.x; x++)
			{
				VoxelColumn& column = tile.myVoxelGrid[z * tile.mySize.x + x];
				for (VoxelSpan& span : column.mySpans)
				{
					if (span.myRegionId != VoxelSpan::kInvalidRegion)
					{
						continue;
					}

					span.myRegionId = ++regionCounter;
					myRegions.push_back({ 
						{},
						{},
						tile.myAABBMin, 
						span.myMaxY, 
						span.myRegionId 
					});
					Region& region = myRegions.back();
					region.mySpans.push_back({ &span, { x, z } });

					spansToCheck.push_back({ x, z });
					while (!spansToCheck.empty())
					{
						FindNeighbours(spansToCheck, region, tile);
					}
				}
			}
		}
	}

	// We generated corner points per tile in isolation meaning
	// tile-edge voxels are missing their neighbors in another tile.
	// The paper tries to fix this with eliminating these "extra" corner
	// points. But I think it's beneficial to have them, as they 
	// create natural points of merging the tiles in the future. 
	// So I'll skip this step for now and see how it goes.
}

void NavMeshGen::ExtractContours()
{
}