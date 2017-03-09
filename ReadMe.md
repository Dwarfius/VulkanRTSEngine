Info
====

This is a mutltithreaded engine project indended as a research in to Vulkan API. The engine features a OpenGL and Vulkan rendering backends with ability to switch them out at will. The engine executes a RTS-battle scenarion until stopped.

Project uses CMake. You'll need VulkanSDK, GLEW, GLM, GLFW and Bullet2(static libraries, more on it bellow). Tested on Windows, should be runnable on Linux too.

In order to use Bullet, build the static libraries (turn off all build features, leave MSVC_FAST_FLOATS, INSTALL_LIBS on).
Project is configured to find Bullet, but in case it doesn't find it, add a variable BULLET_ROOT pointing to the Install location of Bullet.

Tasks
=====
* Start working on collisions

### OpenGL
* Animations
* Add debug drawing (sphere for now)

### Vulkan
* Figure out why can't reuse the swapchain when recreating it on resize
* Terrain doesn't render properly
* Animations
* Figure out why positions are different from OpenGL
* Add debug drawing (sphere for now)