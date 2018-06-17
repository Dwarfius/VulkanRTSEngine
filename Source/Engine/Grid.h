#pragma once

class GameObject;

class Grid
{
public:
	Grid(glm::ivec3 start, glm::ivec3 end, glm::vec3 cellSize);
	~Grid();

	void SetTreadBufferCount(uint32_t threads);
	// enques object to be placed in the grid
	void Add(GameObject *go, uint32_t threadId);
	// clears the grid and flushes the queues in to the grid
	void Flush();

	size_t GetTotal() { return width * height * depth; }
	vector<GameObject*>* GetCell(size_t index) { return grid[index]; }

private:
	glm::vec3 start, end, cellSize;
	uint32_t width, height, depth;
	vector<vector<GameObject*>*> grid;
	struct ResolvedGO
	{
		GameObject *go;
		glm::ivec3 coordsStart;
		glm::ivec3 coordsEnd;
	};
	vector<vector<ResolvedGO>*> buffers;
};