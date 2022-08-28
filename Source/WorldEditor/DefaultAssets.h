#pragma once

#include <Core/RefCounted.h>

class Game;
class Model;
class Texture;
class Pipeline;

class DefaultAssets
{
public:
	DefaultAssets(Game& aGame);

	Handle<Model> GetPlane() const { return myPlane; }
	Handle<Model> GetSphere() const { return mySphere; }
	Handle<Model> GetBox() const { return myBox; }
	Handle<Model> GetCylinder() const { return myCylinder; }
	Handle<Model> GetCone() const { return myCone; }

	Handle<Texture> GetUVTexture() const { return myUVTexture; }

	Handle<Pipeline> GetPipeline() const { return myPipeline; }

private:
	void CreatePlane(Game& aGame);
	void CreateSphere(Game& aGame);
	void CreateBox(Game& aGame);
	void CreateCylinder(Game& aGame);
	void CreateCone(Game& aGame);

	Handle<Model> myPlane;
	Handle<Model> mySphere;
	Handle<Model> myBox;
	Handle<Model> myCylinder;
	Handle<Model> myCone;
	Handle<Texture> myUVTexture;
	Handle<Pipeline> myPipeline;
};