#pragma once

#include "Vertex.h"

class GameObject;
struct GLFWwindow;
class Camera;
class Terrain;

// TODO: need to move resource-load to files
const static vector<string> ourShadersToLoad = {
	"base",
	"debug"
};
const static vector<string> ourModelsToLoad = {
	"cube",
	"%t0", // TODO: need to look into better terrain loading approach
	"Tank",
	"sphere"
};
const static vector<string> ourTexturesToLoad = {
	"chalet.jpg",
	"test.jpg",
	"CubeUnwrap.jpg",
	"wireframe.png",
	"playerTank.png",
	"enemyTank.png",
	"gray.png"
};

// providing unified interfaces (just to make it easier to see/track)
// Model
typedef uint32_t ModelId;
struct Model 
{
	uint32_t myId;
	size_t myVertexCount, myVertexOffset;
	size_t myIndexCount, myIndexOffset;
	glm::vec3 myCenter;
	float mySphereRadius;

	Model()
		: myId(0)
		, myVertexCount(0)
		, myVertexOffset(0)
		, myIndexCount(0)
		, myIndexOffset(0)
		, myCenter(0.f, 0.f, 0.f)
		, mySphereRadius(0)
	{

	}

	// TODO: look into refactoring the GL buffers out to GL internals system - no need to expose this to the user
	// OpenGL resource tracking (VBO + EBO) - has to be here to support vector<Model> Graphics::models
	uint32_t myGLBuffers[2]; 
};

// Shader
typedef uint32_t ShaderId;
struct Shader 
{
	uint32_t myId;

	enum UniformType { 
		Int, Float, Vec2, 
		Vec3, Vec4, Mat4 
	};
	union UniformValue {
		int32_t myInt;
		float myFloat;
		glm::vec2 myV2;
		glm::vec3 myV3;
		glm::vec4 myV4;
		glm::mat4 myMatrix;

		UniformValue() {}
	};
	struct BindPoint 
	{ 
		GLint myLocation; 
		UniformType myType;
	};
	unordered_map<string, BindPoint> myUniforms;

	// TODO: refactor this out to GL system
	// OpenGL resource tracking - Frag and Vert shaders
	uint32_t myGLSources[2]; 
};

// Texture
typedef uint32_t TextureId;

class Graphics
{
public:
	virtual void Init(const vector<Terrain*>& aTerrainList) = 0;
	virtual void BeginGather() = 0;
	virtual void Render(const Camera& aCam, const GameObject* aGO) = 0;
	virtual void Display() = 0;
	virtual void CleanUp() = 0;
	
	glm::vec3 GetModelCenter(ModelId aModelId) const { return myModels[aModelId].myCenter; }
	float GetModelRadius(ModelId aModelId) const { return myModels[aModelId].mySphereRadius; }

	GLFWwindow* GetWindow() const { return myWindow; }
	
	uint32_t GetRenderCalls() const { return myRenderCalls; }
	void ResetRenderCalls() { myRenderCalls = 0; }

	static float GetWidth() { return static_cast<float>(ourWidth); }
	static float GetHeight() { return static_cast<float>(ourHeight); }

	// TODO: replace aName with aPath
	static void LoadModel(string aName, vector<Vertex>& aVertices, vector<uint32_t>& anIndices, glm::vec3& aCenter, float& aRadius);
	static unsigned char* LoadTexture(string aName, int* aWidth, int* aHeight, int* aChannels, int aDesiredChannels);
	static void FreeTexture(void* aData);

	// Notifies the rendering system about how many threads will access it
	virtual void SetMaxThreads(uint32_t aMaxThreadCount) {};

protected:
	GLFWwindow* myWindow;
	static int ourWidth, ourHeight;

	static Graphics* ourActiveGraphics;

	vector<Model> myModels;

	uint32_t myRenderCalls;

	string ReadFile(const string& filename) const;

	// need to have this copy here so that classes that 
	// inherit from Graphics know what's available and 
	// are not tied to stb. yes, it's not kosher, sorry.
	enum
	{
		STBI_default = 0, // only used for req_comp

		STBI_grey = 1,
		STBI_grey_alpha = 2,
		STBI_rgb = 3,
		STBI_rgb_alpha = 4
	};
};
