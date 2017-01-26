#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <vector>
#include <string>

using namespace std;

const std::vector<string> shadersToLoad = {
	"base"
};
const std::vector<string> modelsToLoad = {

};
const std::vector<string> texturesToLoad = {

};

// providing unified interfaces (just to make it easier to see/track)
typedef uint32_t ModelId;
typedef uint32_t ShaderId;
typedef uint32_t TextureId;

// forward declaring to resolve a circular dependency
class GameObject;

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
