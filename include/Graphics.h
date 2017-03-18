#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "Camera.h"
#include <vector>
#include <string>
#include <unordered_map>
#include "Vertex.h"

using namespace std;
using namespace glm;

const std::vector<string> shadersToLoad = {
	"base",
	"debug"
};
const std::vector<string> modelsToLoad = {
	//"chalet",
	"cube",
	"%t0"
};
const std::vector<string> texturesToLoad = {
	"chalet.jpg",
	"test.jpg",
	"CubeUnwrap.jpg"
};

const int maxThreads = 16;

// providing unified interfaces (just to make it easier to see/track)
typedef uint32_t ModelId;
struct Model 
{
	uint32_t id;
	size_t vertexCount, vertexOffset;
	size_t indexCount, indexOffset;
	vec3 center;
	float sphereRadius;

	vector<uint32_t> buffers; // OpenGL resource tracking
};

typedef uint32_t ShaderId;
struct Shader 
{
	uint32_t id;

	enum UniformType { Int, Float, Vec2, Vec3, Vec4, Mat4 };
	union UniformValue {
		int32_t i;
		float f;
		vec2 v2;
		vec3 v3;
		vec4 v4;
		mat4 m;

		UniformValue() {}
	};
	struct BindPoint 
	{ 
		uint loc; 
		UniformType type;
	};
	unordered_map<string, BindPoint> uniforms;

	vector<uint32_t> shaderSources; // OpenGL resource tracking
};

typedef uint32_t TextureId;

// forward declaring to resolve a circular dependency
class GameObject;
struct GLFWwindow;
class Terrain;

class Graphics
{
public:
	virtual void Init(vector<Terrain> terrains) = 0;
	virtual void BeginGather() = 0;
	virtual void Render(const Camera *cam, GameObject *go, const uint32_t threadId) = 0;
	virtual void Display() = 0;
	virtual void CleanUp() = 0;
	
	virtual vec3 GetModelCenter(ModelId m) = 0;

	GLFWwindow* GetWindow() { return window; }
	
	int GetRenderCalls() const;
	void ResetRenderCalls();

	float GetWidth() { return width; }
	float GetHeight() { return height; }

	static void LoadModel(string name, vector<Vertex> &vertices, vector<uint32_t> &indices, vec3 &center, float &radius);
	static unsigned char* LoadTexture(string name, int *x, int *y, int *channels, int desiredChannels);
	static void FreeTexture(void *data);

protected:
	GLFWwindow *window;
	float width = 800, height = 600;

	static Graphics *activeGraphics;
	static ModelId currModel;
	static ShaderId currShader;
	static TextureId currTexture;

	int renderCalls[maxThreads];

	string readFile(const string& filename);

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
#endif // !_GRAPHICS_H
