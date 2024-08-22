#pragma once

// Avoids removing-then-adding to the same cells in Move().
// This can save on cache thrashing if the Grid contains many
// multi-cell objects
//#define GRID_MOVE_OVERLAP

template<class T>
class Grid
{
public:
	Grid(glm::vec2 aMin, float aGridSize, uint16_t aCellCount)
		: myMin(aMin)
		, myGridSize(aGridSize)
		, myCellSize(aGridSize / aCellCount)
		, myCellCount(aCellCount)
	{
		myGridCells.resize(myCellCount * myCellCount);
	}

	void Add(glm::vec2 aMin, glm::vec2 aMax, T anItem)
	{
		const glm::u16vec2 minCoords = GetCellIndices(aMin);
		const glm::u16vec2 maxCoords = GetCellIndices(aMax);
		AddInternal(minCoords, maxCoords, anItem);
	}

	void Remove(glm::vec2 aMin, glm::vec2 aMax, T anItem)
	{
		const glm::u16vec2 minCoords = GetCellIndices(aMin);
		const glm::u16vec2 maxCoords = GetCellIndices(aMax);
		RemoveInternal(minCoords, maxCoords, anItem);
	}

	void Move(glm::vec2 anOldMin, glm::vec2 anOldMax, glm::vec2 aMin, glm::vec2 aMax, T anItem)
	{
		const glm::u16vec2 oldMinCoords = GetCellIndices(anOldMin);
		const glm::u16vec2 oldMaxCoords = GetCellIndices(anOldMax);

		const glm::u16vec2 minCoords = GetCellIndices(aMin);
		const glm::u16vec2 maxCoords = GetCellIndices(aMax);

		if (oldMinCoords == minCoords && oldMaxCoords == maxCoords)
		{
			return;
		}

#ifdef GRID_MOVE_OVERLAP
		// TODO: move to utils
		if (oldMinCoords.x <= maxCoords.x && oldMaxCoords.x >= minCoords.x
			&& oldMinCoords.y <= maxCoords.y && oldMaxCoords.y >= minCoords.y)
		{
			// there's overlap - skip the overlap sections
			for (uint16_t y = oldMinCoords.y; y <= oldMaxCoords.y; y++)
			{
				for (uint16_t x = oldMinCoords.x; x <= oldMaxCoords.x; x++)
				{
					// Avoid removing what will be readded
					if (x >= minCoords.x && x <= maxCoords.x
						&& y >= minCoords.y && y <= maxCoords.y)
					{
						continue;
					}
					const uint32_t index = y * myCellCount + x;
					RemoveFromCell(index, anItem);
				}
			}

			for (uint16_t y = minCoords.y; y <= maxCoords.y; y++)
			{
				for (uint16_t x = minCoords.x; x <= maxCoords.x; x++)
				{
					// avoid adding what would've been re-added
					if (x >= oldMinCoords.x && x <= oldMaxCoords.x
						&& y >= oldMinCoords.y && y <= oldMaxCoords.y)
					{
						continue;
					}
					const uint32_t index = y * myCellCount + x;
					AddToCell(index, anItem);
				}
			}
		}
		else
#endif
		{
			// no overlap - do the full routine
			RemoveInternal(oldMinCoords, oldMaxCoords, anItem);
			AddInternal(minCoords, maxCoords, anItem);
		}
	}

	template<class TFunc>
	void ForEachCell(TFunc&& aFunc)
	{
		for(uint32_t i=0; i<myGridCells.size(); i++)
		{
			std::vector<T>& cell = myGridCells[i];
			if constexpr (requires(glm::vec2 a) { aFunc(a, a, cell); })
			{
				const uint16_t y = i / myCellCount;
				const uint16_t x = i % myCellCount;
				const glm::vec2 min = myMin + glm::vec2{ x * myCellSize, y * myCellSize };
				const glm::vec2 max = min + glm::vec2{ myCellSize };
				aFunc(min, max, cell);
			}
			else
			{
				aFunc(cell);
			}
		}
	}

	template<class TFunc>
	void ForEachCell(glm::vec2 aMin, glm::vec2 aMax, TFunc&& aFunc)
	{
		const glm::u16vec2 minIndices = GetCellIndices(aMin);
		const glm::u16vec2 maxIndices = GetCellIndices(aMax);
		for (uint16_t y = minIndices.y; y <= maxIndices.y; y++)
		{
			for (uint16_t x = minIndices.x; x <= maxIndices.x; x++)
			{
				std::vector<T>& cell = myGridCells[y * myCellCount + x];
				aFunc(cell);
			}
		}
	}

	void Clear()
	{
		for (std::vector<T>& cell : myGridCells)
		{
			cell.clear();
		}
	}

private:
	// TODO: implement simd to do both min and max in 1 go
	glm::u16vec2 GetCellIndices(glm::vec2 aPos) const 
	{  
		const glm::vec2 offset = aPos - myMin;
		glm::i32vec2 coords = static_cast<glm::i32vec2>(offset / myCellSize);
		coords = glm::clamp(coords, glm::i32vec2{ 0 }, glm::i32vec2{ myCellCount }); 
		return static_cast<glm::u16vec2>(coords);
	}

	void AddInternal(glm::u16vec2 aMinCoords, glm::u16vec2 aMaxCoords, T anItem)
	{
		for (uint16_t y = aMinCoords.y; y <= aMaxCoords.y; y++)
		{
			for (uint16_t x = aMinCoords.x; x <= aMaxCoords.x; x++)
			{
				const uint32_t index = y * myCellCount + x;
				AddToCell(index, anItem);
			}
		}
	}

	void AddToCell(uint32_t anIndex, T anItem)
	{
		myGridCells[anIndex].push_back(anItem);
	}

	void RemoveInternal(glm::u16vec2 aMinCoords, glm::u16vec2 aMaxCoords, T anItem)
	{
		for (uint16_t y = aMinCoords.y; y <= aMaxCoords.y; y++)
		{
			for (uint16_t x = aMinCoords.x; x <= aMaxCoords.x; x++)
			{
				const uint32_t index = y * myCellCount + x;
				RemoveFromCell(index, anItem);
			}
		}
	}

	void RemoveFromCell(uint32_t anIndex, T anItem)
	{
		std::vector<T>& cell = myGridCells[anIndex];
		if (cell.size() > 1)
		{
			auto iter = std::ranges::find(cell, anItem);
			ASSERT(iter != cell.end());
			std::swap(*iter, cell[cell.size() - 1]);
		}
		ASSERT(cell[cell.size() - 1] == anItem);
		cell.pop_back();
	}

	std::vector<std::vector<T>> myGridCells;
	glm::vec2 myMin;
	float myGridSize;
	float myCellSize;
	uint16_t myCellCount;
};

#undef GRID_MOVE_OVERLAP