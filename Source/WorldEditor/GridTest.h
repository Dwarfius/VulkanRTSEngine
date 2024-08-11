#pragma once

#include <Core/Grid.h>
#include <Core/StableVector.h>

class Game;

class GridTest
{
public:
	GridTest();

	void DrawTab();
	void DrawGrid(Game& aGame);

private:
	enum class Mode
	{
		None,
		Add,
		Erase
	};

	struct Point
	{
		glm::vec2 myPos;
		float mySize;
		bool myIsBeingDeleted;
	};

	StableVector<Point> myPoints;
	Grid<Point*> myGrid;
	Mode myMode = Mode::None;
	float myCreateSize = 1.f;
};