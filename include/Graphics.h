#ifndef _GRAPHICS_H
#define _GRAPHICS_H

// this is just a include-resolver plug to unify includes across the project
#if defined(GRAPHICS_VK)
	#include "GraphicsVK.h"
#elif defined(GRAPHICS_GL)
	#include "GraphicsGL.h"
#endif

/* Each of the headers implements a Graphics class with static methods. Available methods are:
		Init() - performs back-end initialization
		Render() - renders everything to target buffer
		Display() - presents the rendered frame to the screen
		CleanUp() - performs resource cleanup
		GetWindow() - to get a handle to created GLFW window
*/

#endif // !_GRAPHICS_H
