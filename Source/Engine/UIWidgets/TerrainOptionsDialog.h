#pragma once

class Terrain;
class Game;

class TerrainOptionsDialog
{
public:
	void Draw(Game& aGame, bool& aIsVisible);

private:
	void DrawTerrain(Terrain& aTerrain);

	glm::vec3 myNewColor { 1, 1, 1 };
	float myNewHeight = 0;
};