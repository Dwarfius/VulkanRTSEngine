#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "Camera.h"
#include <vector>
#include <string>
#include <glm/gtx/hash.hpp>

using namespace std;
using namespace glm;

const std::vector<string> shadersToLoad = {
	"base",
	"debug"
};
const std::vector<string> modelsToLoad = {
	"chalet",
	"cube"
};
const std::vector<string> texturesToLoad = {
	"chalet.jpg",
	"test.jpg"
};

// providing unified interfaces (just to make it easier to see/track)
typedef uint32_t ModelId;
struct Model 
{
	uint32_t id;
	size_t vertexCount;
	size_t indexCount;
	vec3 center;

	vector<uint32_t> buffers; // OpenGL resource tracking
};

typedef uint32_t ShaderId;
struct Shader 
{
	uint32_t id;
	vector<uint32_t> shaderSources; // OpenGL resource tracking
};

typedef uint32_t TextureId;

// forward declaring to resolve a circular dependency
class GameObject;
struct GLFWwindow;

struct Vertex 
{
	vec3 pos;
	vec2 uv;
	vec3 normal;

	bool operator==(const Vertex& other) const 
	{
		return pos == other.pos && uv == other.uv && normal == other.normal;
	}
};

namespace std 
{
	template<> struct hash<Vertex> 
	{
		size_t operator()(Vertex const& vertex) const 
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}

class Graphics
{
public:
	virtual void Init() = 0;
	virtual void Render(Camera *cam, GameObject *go, uint threadId) = 0;
	virtual void Display() = 0;
	virtual void CleanUp() = 0;
	
	virtual vec3 GetModelCenter(ModelId m) = 0;

	GLFWwindow* GetWindow() { return window; }
protected:
	GLFWwindow *window;

	static ModelId currModel;
	static ShaderId currShader;
	static TextureId currTexture;

	void LoadModel(string name, vector<Vertex> &vertices, vector<uint> &indices, vec3 &center);
	unsigned char* LoadTexture(string name, int *x, int *y, int *channels, int desiredChannels);
	void FreeTexture(void *data);
	string readFile(const string& filename);

	// need to have this copy here so that classes that 
	// inherit from Graphics know what's available and 
	// are not tied to stb
	enum
	{
		STBI_default = 0, // only used for req_comp

		STBI_grey = 1,
		STBI_grey_alpha = 2,
		STBI_rgb = 3,
		STBI_rgb_alpha = 4
	};
};

// this is just a include-resolver plug to unify includes across the project
#if defined(GRAPHICS_VK)
	#include "GraphicsVK.h"
#elif defined(GRAPHICS_GL)
	#include "GraphicsGL.h"
#endif

/* Each of the headers implements a Graphics class with static methods. Available methods are:
		Init() - performs back-end initialization
		Render(GameObject*) - renders everything to target buffer
		Display() - presents the rendered frame to the screen
		CleanUp() - performs resource cleanup
		GetWindow() - to get a handle to created GLFW window
*/

#endif // !_GRAPHICS_H
