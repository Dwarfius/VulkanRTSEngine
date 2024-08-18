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

	constexpr uint8_t kScenario = 0;
	Tile& tile = myTiles[2];
	const uint32_t width = tile.mySize.x;
	const uint32_t halfHeight = static_cast<uint32_t>(
		(tile.myMaxHeight - tile.myMinHeight) / kVoxelHeight / 2
	);
	switch (kScenario)
	{
	case 0:
		GatherTriangles(aGame.GetAssetTracker());
		break;
	case 1:
		tile.myVoxelGrid[0].AddBoth(halfHeight);
		tile.myVoxelGrid[1].AddBoth(halfHeight);
		tile.myVoxelGrid[2].AddBoth(halfHeight);
		tile.myVoxelGrid[width].AddBoth(halfHeight);
		tile.myVoxelGrid[width + 1].AddBoth(halfHeight);
		tile.myVoxelGrid[width + 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 1].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 2].AddBoth(halfHeight);
		break;
	case 2:
		tile.myVoxelGrid[0].AddBoth(halfHeight);
		tile.myVoxelGrid[1].AddBoth(halfHeight);
		tile.myVoxelGrid[2].AddBoth(halfHeight);
		tile.myVoxelGrid[width].AddBoth(halfHeight);
		tile.myVoxelGrid[width + 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 1].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 2].AddBoth(halfHeight);
		break;
	case 3:
		tile.myVoxelGrid[0].AddBoth(halfHeight);
		tile.myVoxelGrid[1].AddBoth(halfHeight);
		tile.myVoxelGrid[2].AddBoth(halfHeight);
		tile.myVoxelGrid[3].AddBoth(halfHeight);
		tile.myVoxelGrid[width].AddBoth(halfHeight);
		tile.myVoxelGrid[width + 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width + 3].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 1].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 3].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 3].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 3 + 1].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 3 + 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 3 + 3].AddBoth(halfHeight);
		break;
	case 4:
		tile.myVoxelGrid[0].AddBoth(halfHeight);
		tile.myVoxelGrid[1].AddBoth(halfHeight);
		tile.myVoxelGrid[2].AddBoth(halfHeight);
		tile.myVoxelGrid[3].AddBoth(halfHeight);
		tile.myVoxelGrid[4].AddBoth(halfHeight);
		tile.myVoxelGrid[width].AddBoth(halfHeight);
		tile.myVoxelGrid[width + 1].AddBoth(halfHeight);
		tile.myVoxelGrid[width + 3].AddBoth(halfHeight);
		tile.myVoxelGrid[width + 4].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 3].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 2 + 4].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 3].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 3 + 1].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 3 + 4].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 4].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 4 + 1].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 4 + 2].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 4 + 3].AddBoth(halfHeight);
		tile.myVoxelGrid[width * 4 + 4].AddBoth(halfHeight);
		break;
	}

	SegmentTiles();
	ExtractContours();
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

	if (mySettings.myDrawContours)
	{
		for (const Contour& contour : myContours)
		{
			contour.Draw(aDrawer);
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

void NavMeshGen::Region::GatherCornerVerts(const std::vector<SpanPos>& aSpans)
{
	auto ConvertToWS = [](glm::vec3 anOrigin, glm::u32vec2 aPos, uint32_t aHeight, uint8_t aVert)
	{
		constexpr static glm::vec3 kOffsets[]{
			{0, 0, kVoxelSize},
			{kVoxelSize, 0, kVoxelSize},
			{0, 0, 0},
			{kVoxelSize, 0, 0}
		};
		const glm::vec3 spanOrigin = anOrigin + glm::vec3{
			aPos.x * kVoxelSize,
			aHeight * kVoxelHeight,
			aPos.y * kVoxelSize
		};
		return spanOrigin + kOffsets[aVert];
	};

	struct CornerVertHash 
	{
		std::size_t operator()(const CornerVert& aVert) const noexcept
		{
			// We only want to deduplicate corners in a region, so we can 
			// ignore the height and the type of corner. Additionally, because 
			// float innacuracies can creep in we protect against it by 
			// preserving only the significant part
			static_assert(kVoxelSize >= 0.01f && kVoxelSize < 0.1f, 
				"Update multiplier bellow!");
			const glm::u32vec2 pos{
				static_cast<uint32_t>(glm::round(aVert.myPos.x * 100)),
				static_cast<uint32_t>(glm::round(aVert.myPos.z * 100))
			};
			return std::hash<glm::u32vec2>{}(pos);
		}
	};

	struct CornerVertEq
	{
		bool operator()(const CornerVert& aLeft, const CornerVert& aRight) const noexcept 
		{
			static_assert(kVoxelSize >= 0.01f && kVoxelSize < 0.1f,
				"Update multiplier bellow!");
			const glm::u32vec2 left{
				static_cast<uint32_t>(glm::round(aLeft.myPos.x * 100)),
				static_cast<uint32_t>(glm::round(aLeft.myPos.z * 100))
			};
			const glm::u32vec2 right{
				static_cast<uint32_t>(glm::round(aRight.myPos.x * 100)),
				static_cast<uint32_t>(glm::round(aRight.myPos.z * 100))
			};
			return left == right;
		}
	};
	std::unordered_set<CornerVert, CornerVertHash, CornerVertEq> slashPoints;
	for (const SpanPos& span : aSpans)
	{
		for (uint8_t i = 0; i < 4; i++)
		{
			if (IsVertexSlashPoint(i, span.mySpan->myNeighbors))
			{
				CornerVert vert{
					ConvertToWS(myTileAABBMin, span.myPos, myHeight, i),
					(i == 1 || i == 2) ? 2u : 1u
				};
				if (slashPoints.contains(vert))
				{
					continue;
				}
				slashPoints.insert(vert);
				myCornerVerts.push_back(vert);
			}
			else if(IsVertexCorner(i, span.mySpan->myNeighbors))
			{
				CornerVert vert{
					ConvertToWS(myTileAABBMin, span.myPos, myHeight, i),
					0
				};
				if (slashPoints.contains(vert))
				{
					continue;
				}
				slashPoints.insert(vert);
				myCornerVerts.push_back(vert);
			}
		}
	}
}

void NavMeshGen::CornerVert::Draw(DebugDrawer& aDrawer) const
{
	const glm::vec3 color = myType == 0 ? glm::vec3{ 0, 1, 0 }
		: myType == 2 ? glm::vec3{ 1, 0, 0 } : glm::vec3{ 0, 0, 1 };
	aDrawer.AddSphere(
		myPos,
		kVoxelSize / 5.f,
		color
	);
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
		const glm::vec3 p3 = p2 + glm::vec3{ 0, 0, kVoxelSize };
		const glm::vec3 p4 = p1 + glm::vec3{ 0, 0, kVoxelSize };
		aDrawer.AddRect(p1, p2, p3, p4, color);
	}
}

void NavMeshGen::Region::DrawCornerPoints(DebugDrawer& aDrawer) const
{
	for (const CornerVert& vert : myCornerVerts)
	{
		vert.Draw(aDrawer);
	}
}

void NavMeshGen::SegmentTiles()
{
	constexpr static uint8_t kNeighborEncTable[]{
		32, 64, 128,
		8, 0, 16,
		1, 2, 4
	};

	auto FindNeighbours = [](std::vector<glm::u32vec2>& aToCheck, Region& aRegion, 
		std::vector<Region::SpanPos>& aCornerSpans, Tile& aTile) {
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

				aCornerSpans.push_back({ &span, spanIndex });
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

		std::vector<Region::SpanPos> cornerSpans;
		cornerSpans.reserve(64); // arbitrary
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
						FindNeighbours(spansToCheck, region, cornerSpans, tile);
					}

					region.GatherCornerVerts(cornerSpans);
					cornerSpans.clear();
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

void NavMeshGen::Contour::Draw(DebugDrawer& aDrawer) const
{
	for (uint32_t i = 0; i < myVerts.size() - 1; i++)
	{
		const CornerVert& start = myVerts[i];
		const CornerVert& end = myVerts[i + 1];
		aDrawer.AddLine(start.myPos, end.myPos, { 0, 0, 1 });
	}
	aDrawer.AddLine(myVerts.back().myPos, myVerts.front().myPos, { 0, 0, 1 });
}

void NavMeshGen::ExtractContours()
{
	Profiler::ScopedMark scope("NavMeshGen::ExtractContours");

	struct CornerPoint
	{
		glm::vec3 myPos;
		uint8_t myType : 2; // 0 - normal, 1 - antislash, 2 - slash
		uint8_t myIsConnected : 1; // false, true

		CornerPoint(const CornerVert& aVert)
			: myPos(aVert.myPos)
			, myType(aVert.myType)
			, myIsConnected(0)
		{
		}
	};

	std::vector<CornerPoint> cornerPoints;
	std::vector<std::vector<CornerPoint*>> rows;
	rows.resize(kVoxelsPerTile);
	std::vector<std::vector<CornerPoint*>> columns;
	columns.resize(kVoxelsPerTile);
	std::unordered_map<CornerPoint*, CornerPoint*> neighboursHor;
	std::unordered_map<CornerPoint*, CornerPoint*> neighboursVer;
	for (const Region& region : myRegions)
	{
		cornerPoints.clear();
		for (std::vector<CornerPoint*>& row : rows)
		{
			row.clear();
		}
		for (std::vector<CornerPoint*>& column : columns)
		{
			column.clear();
		}
		neighboursHor.clear();
		neighboursVer.clear();

		// reserving to avoid relocations - *2 covers worst case
		cornerPoints.reserve(region.myCornerVerts.size() * 2); 
		for (const CornerVert& cornerVert : region.myCornerVerts)
		{
			const glm::u32vec2 gridPos{
				glm::round((cornerVert.myPos.x - region.myTileAABBMin.x) / kVoxelSize),
				glm::round((cornerVert.myPos.z - region.myTileAABBMin.z) / kVoxelSize)
			};
			if (cornerVert.myType != 0)
			{
				CornerPoint& p1 = cornerPoints.emplace_back(cornerVert);
				CornerPoint& p2 = cornerPoints.emplace_back(cornerVert);
				rows[gridPos.y].push_back(&p1);
				rows[gridPos.y].push_back(&p2);

				columns[gridPos.x].push_back(&p1);
				columns[gridPos.x].push_back(&p2);
			}
			else
			{
				CornerPoint& p = cornerPoints.emplace_back(cornerVert);
				rows[gridPos.y].push_back(&p);
				columns[gridPos.x].push_back(&p);
			}
		}

		// sort so that we can use the even-odd rule (Jordan Theorem)
		for (std::vector<CornerPoint*>& row : rows)
		{
			std::sort(row.begin(), row.end(), [](const auto& aLeft, const auto& aRight) {
				return aLeft->myPos.x < aRight->myPos.x;
			});
		}
		for (std::vector<CornerPoint*>& column : columns)
		{
			std::sort(column.begin(), column.end(), [](const auto& aLeft, const auto& aRight) {
				return aLeft->myPos.z < aRight->myPos.z;
			});
		}

		// Gather neighhors
		for (const std::vector<CornerPoint*>& row : rows)
		{
			if (row.empty())
			{
				continue;
			}

			// Even-only
			for (uint32_t i = 0; i < row.size() - 1; i += 2)
			{
				neighboursHor.insert({ row[i], row[i + 1] });
				neighboursHor.insert({ row[i + 1], row[i] });
			}
		}
		for (const std::vector<CornerPoint*>& column : columns)
		{
			if (column.empty())
			{
				continue;
			}

			// Even only
			for (uint32_t i = 0; i < column.size() - 1; i += 2)
			{
				CornerPoint& s1 = *column[i];
				CornerPoint& s2 = *column[i + 1];

				// format is a -> b and p is normal point,
				// p1 stands for first of dupe pair, p2 - second
				CornerPoint* a = nullptr;
				CornerPoint* b = nullptr;
				if (s1.myType == 0 && s2.myType == 0)
				{
					a = &s1;
					b = &s2;
				}
				else if (s1.myType == 1 || s1.myType == 2) // antislash or slash point
				{
					if (s1.myType == 1) // antislash point
					{
						if (s2.myType == 0) // normal
						{
							// p2 -> p
							a = &s1;
							b = &s2;
						}
						else // can only be slash
						{
							// p2 -> p2
							a = &s1;
							b = column[i + 2];
						}
					}
					else // slash point
					{
						if (s2.myType == 0)
						{
							// p1 -> p
							a = column[i - 1];
							b = &s2;
						}
						else // can only be antislash
						{
							// p1 -> p1
							a = column[i - 1];
							b = &s2;
						}
					}
				}
				else // s1 normal, s2 is slash or antislash
				{
					if (s2.myType == 1) // antislash
					{
						// p -> p1
						a = &s1;
						b = &s2;
					}
					else // slash
					{
						// p -> p2
						a = &s1;
						b = column[i + 2];
					}
				}
				ASSERT(a != nullptr && b != nullptr && a != b);
				neighboursVer.insert({ a, b });
				neighboursVer.insert({ b, a });
			}
		}
		// Connect contour vertices
		for(CornerPoint& point : cornerPoints)
		{
			CornerPoint* start = &point;
			if (start->myIsConnected)
			{
				continue;
			}

			Contour& cont = myContours.emplace_back();
			cont.myRegion = &region;
			cont.myVerts.push_back({ start->myPos, start->myType });
			
			CornerPoint* next = neighboursVer[start];
			while (next != start)
			{
				ASSERT(next->myIsConnected == 0);
				cont.myVerts.push_back({ next->myPos, next->myType });
				next->myIsConnected = 1;

				// avoid looping back
				if (CornerPoint* nextH = neighboursHor[next]; nextH->myIsConnected == 0)
				{
					next = nextH;
				}
				else
				{
					next = neighboursVer[next];
				}
			}
			start->myIsConnected = 1;
		}
	}
}