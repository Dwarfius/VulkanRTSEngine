#include "Grid.h"
#include "Game.h"

Grid::Grid(vec3 start, vec3 end, vec3 cellSize) : start(start), end(end), cellSize(cellSize)
{
	width = (end.x - start.x) / cellSize.x;
	height = (end.y - start.y) / cellSize.y;
	depth = (end.z - start.z) / cellSize.z;

	uint32_t total = GetTotal();
	grid.resize(total);
	for (uint32_t i = 0; i < total; i++)
	{
		grid[i] = new vector<GameObject*>();
		grid[i]->reserve(2000); // prealloc just to be safe
	}
}

Grid::~Grid()
{
	uint32_t total = GetTotal();
	for (uint32_t i = 0; i < total; i++)
		delete grid[i];
	grid.clear();
}

void Grid::SetTreadBufferCount(uint32_t threads)
{
	buffers.resize(threads);
	for (uint32_t i = 0; i < threads; i++)
		buffers[i]->reserve(4000);
}

void Grid::Add(GameObject *go, uint32_t threadId)
{
	// getting our bounding sphere definition
	vec3 loc = go->GetTransform()->GetPos();
	float radius = go->GetRadius();

	// constucting an AABB around bounding sphere
	// it's inaccurate (inflated volume), but fast and safe
	vec3 AABBStart = loc - vec3(radius, radius, radius);
	vec3 AABBEnd = loc + vec3(radius, radius, radius);

	// constraining coords to edges, if needed
	AABBStart = clamp(AABBStart, start, end);
	AABBEnd = clamp(AABBEnd, start, end);

	// figuring out which cell it goes to
	vec3 coordsStart = vec3(
		floor(AABBStart.x / cellSize.x),
		floor(AABBStart.y / cellSize.y),
		floor(AABBStart.z / cellSize.z)
	);
	vec3 coordsEnd = vec3(
		floor(AABBEnd.x / cellSize.x),
		floor(AABBEnd.y / cellSize.y),
		floor(AABBEnd.z / cellSize.z)
	);

	// saving processed game object
	// each threads populates it's buffer to avoid write races
	ResolvedGO resGO{ go, coordsStart, coordsEnd };
	buffers[threadId]->push_back(resGO);
}

void Grid::Flush()
{
	// clearing out all previous objects
	uint32_t total = GetTotal();
	for (uint32_t i = 0; i < total; i++)
		grid[i]->clear();

	// going through each queue
	uint32_t total = buffers.size();
	for (uint32_t i = 0; i < total; i++)
	{
		// processing individual queue
		vector<ResolvedGO> *buffer = buffers[i];
		size_t size = buffer->size();
		for (size_t j = 0; j < size; j++)
		{
			// processing resolved game object from the queue - adding it to respective cells
			ResolvedGO r = buffer->at(j);
			for (uint32_t x = r.coordsStart.x; x <= r.coordsEnd.x; x++)
				for (uint32_t y = r.coordsStart.y; y <= r.coordsEnd.y; y++)
					for (uint32_t z = r.coordsStart.z; z <= r.coordsEnd.z; z++)
						grid[x + width * (y + height * z)]->push_back(r.go);
		}
		// cleanup of the queue
		buffer->clear();
	}
}