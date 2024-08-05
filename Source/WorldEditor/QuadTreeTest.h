#pragma once

// disabled for now since I removed my quadtree, since my impl is limited
#ifdef WE_QUADTREE

#include <Core/QuadTree.h>
#include <Core/StableVector.h>

class Game;

class QuadTreeTest
{
public:
	QuadTreeTest();

	void DrawTab();
	void DrawTree(Game& aGame);

private:
	enum class QuadTreeMode
	{
		None,
		Add,
		Erase
	};

	struct Point
	{
		glm::vec2 myPos;
		float mySize;
		QuadTree<Point>::Info myQuadInfo;
	};

	StableVector<Point> myPoints;
	QuadTree<Point*> myPointsQuadTree;
	QuadTreeMode myQuadTreeMode = QuadTreeMode::None;
	float myCreateSize = 1.f;
};

#endif