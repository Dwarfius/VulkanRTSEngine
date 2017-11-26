#include "Common.h"
#include "Grid.h"
#include "Game.h"
#include "GameObject.h"

Grid::Grid(ivec3 start, ivec3 end, vec3 cellSize) 
	: start(start)
	, end(end)
	, cellSize(cellSize)
{
	width = static_cast<uint32_t>((end.x - start.x) / cellSize.x);
	height = static_cast<uint32_t>((end.y - start.y) / cellSize.y);
	depth = static_cast<uint32_t>((end.z - start.z) / cellSize.z);

	size_t total = GetTotal();
	grid.resize(total);
	for (size_t i = 0; i < total; i++)
	{
		grid[i] = new vector<GameObject*>();
		grid[i]->reserve(2000); // prealloc just to be safe
	}
}

Grid::~Grid()
{
	size_t total = GetTotal();
	for (size_t i = 0; i < total; i++)
		delete grid[i];
	grid.clear();
}

void Grid::SetTreadBufferCount(uint32_t threads)
{
	buffers.resize(threads);
	for (uint32_t i = 0; i < threads; i++)
	{
		buffers[i] = new vector<ResolvedGO>();
		buffers[i]->reserve(4000);
	}
}

// TODO: fix the grid utility
void Grid::Add(GameObject *go, uint32_t threadId)
{
	if (!go->GetCollisionsEnabled())
		return;

	go->PreCollision();

	// getting our bounding sphere definition
	vec3 loc = go->GetTransform()->GetPos();
	float radius = go->GetRadius() * 0.75f; // using a smaller radius to get back a bit of accuracy

	// constucting an AABB around bounding sphere
	// it's inaccurate (inflated volume), but fast and safe
	vec3 AABBStart = loc - vec3(radius, radius, radius);
	vec3 AABBEnd = loc + vec3(radius, radius, radius);

	// constraining coords to edges, if needed
	AABBStart = clamp(AABBStart, start, end);
	AABBEnd = clamp(AABBEnd, start, end);

	// converting to grid local coords
	AABBStart -= start;
	AABBEnd -= start;

	// figuring out which cell it goes to
	vec3 coordsStart = vec3(
		clamp(floor(AABBStart.x / cellSize.x), 0.f, width - 1.f),
		clamp(floor(AABBStart.y / cellSize.y), 0.f, height - 1.f),
		clamp(floor(AABBStart.z / cellSize.z), 0.f, depth - 1.f)
	);
	vec3 coordsEnd = vec3(
		clamp(floor(AABBEnd.x / cellSize.x), 0.f, width - 1.f),
		clamp(floor(AABBEnd.y / cellSize.y), 0.f, height - 1.f),
		clamp(floor(AABBEnd.z / cellSize.z), 0.f, depth - 1.f)
	);

	// saving processed game object
	// each threads populates it's buffer to avoid write races
	ResolvedGO resGO{ go, coordsStart, coordsEnd };
	buffers[threadId]->push_back(resGO);
}

void Grid::Flush()
{
	// clearing out all previous objects
	size_t totalObjsInGrid = GetTotal();
	for (uint32_t i = 0; i < totalObjsInGrid; i++)
		grid[i]->clear();

	// going through each queue
	size_t totalObjs = 0;
	size_t buffersSize = buffers.size();
	for (uint32_t i = 0; i < buffersSize; i++)
	{
		// processing individual queue
		vector<ResolvedGO> *buffer = buffers[i];
		size_t size = buffer->size();
		for (size_t j = 0; j < size; j++)
		{
			// processing resolved game object from the queue - adding it to respective cells
			ResolvedGO r = buffer->at(j);
			for (int x = r.coordsStart.x; x <= r.coordsEnd.x; x++)
				for (int y = r.coordsStart.y; y <= r.coordsEnd.y; y++)
					for (int z = r.coordsStart.z; z <= r.coordsEnd.z; z++)
						grid[x + y * width + z * width * height]->push_back(r.go);
		}
		totalObjs += size;
		// cleanup of the queue
		buffer->clear();
	}
	//printf("[Info] Objects in the grid: %zd\n", totalObjs);
}