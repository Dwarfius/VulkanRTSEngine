#pragma once

class World;
class Game;
class AssetTracker;
class DebugDrawer;

// Based on "Robust and Scalable Navmesh Generation with multiple
// levels and stairs support". Guillaume Saupin, Olivier Roussel 
// and Jeremie Le Garrec, 2013 paper
class NavMeshGen
{
	constexpr static float kVoxelSize = 0.05f; // 5cm
	constexpr static float kVoxelHeight = 0.01f; // 1cm
	constexpr static uint32_t kVoxelsPerTile = 256; // 12.8m
public:
	struct Settings
	{
		float myMaxSlope; // deg, how steep it is to be impassable
		float myMinFreeHeight; // NYI, how much free space between floor and ceil is needed

		// Debug
		bool myDrawGenAABB = false;
		bool myDrawValidTriangleChecks = false;
		bool myDrawGeneratedSpans = false;
		bool myDrawRegions = false;
	};

	struct Input
	{
		World* myWorld;
		glm::vec3 myMin;
		glm::vec3 myMax;
	};

public:
	void Generate(const Input& anInput, const Settings& aSettings, Game& aGame);
	void DebugDraw(DebugDrawer& aDrawer) const;

private:
	Settings mySettings;
	Input myInput;

	struct VoxelSpan
	{
		constexpr static uint16_t kInvalidRegion = 0;

		uint32_t myMinY; // discrete from Tile's myMinHeight and myMaxHeight
		uint32_t myMaxY;
		uint16_t myRegionId;
	};

	struct VoxelColumn
	{
		std::vector<VoxelSpan> mySpans;

		void AddTop(uint32_t aHeight);
		void AddBottom(uint32_t aHeight);
		void AddBoth(uint32_t aHeight);
	};

	struct Tile
	{
		std::vector<VoxelColumn> myVoxelGrid;
		glm::u32vec3 mySize; // in voxels
		float myMinHeight;
		glm::vec3 myAABBMin;
		float myMaxHeight;

		void Insert(glm::vec3 aV1, glm::vec3 aV2, glm::vec3 aV3);

		// Debug
		struct Line
		{
			glm::vec3 myStart;
			glm::vec3 myEnd;
			glm::vec3 myColor;
		};
		std::vector<Line> myDebugTriangles;
		void DrawValidTriangleChecks(DebugDrawer& aDrawer) const;
		void DrawVoxelSpans(DebugDrawer& aDrawer) const;
	};
	std::vector<Tile> myTiles;

	void CreateTiles();
	void GatherTriangles(AssetTracker& anAssetTracker);

	struct Region
	{
		struct SpanPos
		{
			VoxelSpan* mySpan;
			glm::u32vec2 myPos; // Note: only for debug drawing
		};
		std::vector<SpanPos> mySpans;
		glm::vec3 myTileAABBMin; // Note: only for debug drawing
		uint32_t myHeight;
		uint16_t myRegionId;

		void Draw(DebugDrawer& aDrawer) const;
	};
	std::vector<Region> myRegions;

	void SegmentTiles();

	void ExtractContours();
	void MergeTiles();
	void BuildNavGraph();
};