#ifndef _GRID_H
#define _GRID_H

#include <vector>
#include <glm\glm.hpp>

using namespace std;
using namespace glm;

class GameObject;

class Grid
{
public:
	Grid(vec3 start, vec3 end, vec3 cellSize);
	~Grid();

	void SetTreadBufferCount(uint32_t threads);
	// enques object to be placed in the grid
	void Add(GameObject *go, uint32_t threadId);
	// clears the grid and flushes the queues in to the grid
	void Flush();

	uint32_t GetTotal() { return width * height * depth; }
	vector<GameObject*>* GetCell(uint32_t index) { return grid[index]; }

private:
	vec3 start, end, cellSize;
	uint32_t width, height, depth;
	vector<vector<GameObject*>*> grid;
	struct ResolvedGO
	{
		GameObject *go;
		vec3 coordsStart;
		vec3 coordsEnd;
	};
	vector<vector<ResolvedGO>*> buffers;
};

#endif