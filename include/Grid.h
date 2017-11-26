#pragma once

class GameObject;

class Grid
{
public:
	Grid(ivec3 start, ivec3 end, vec3 cellSize);
	~Grid();

	void SetTreadBufferCount(uint32_t threads);
	// enques object to be placed in the grid
	void Add(GameObject *go, uint32_t threadId);
	// clears the grid and flushes the queues in to the grid
	void Flush();

	size_t GetTotal() { return width * height * depth; }
	vector<GameObject*>* GetCell(size_t index) { return grid[index]; }

private:
	vec3 start, end, cellSize;
	uint32_t width, height, depth;
	vector<vector<GameObject*>*> grid;
	struct ResolvedGO
	{
		GameObject *go;
		ivec3 coordsStart;
		ivec3 coordsEnd;
	};
	vector<vector<ResolvedGO>*> buffers;
};