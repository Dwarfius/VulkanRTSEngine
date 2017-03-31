Info
====

This is a mutltithreaded engine project indended as a research in to Vulkan API. The engine features a OpenGL and Vulkan rendering backends with ability to switch them out at will. The engine executes a RTS-battle scenarion until stopped.

Project uses CMake. You'll need VulkanSDK, GLEW, GLM, GLFW and Intel's TBB. Tested on Windows, should be runnable on Linux too.

Intel TBB can be fast found using TBB_ROOT_DIR entry.

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
* Add debug drawing (sphere for now)