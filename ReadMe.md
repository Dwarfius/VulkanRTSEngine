Info
====

This is a mutltithreaded engine project indended as a research in to Vulkan API. The engine features a OpenGL and Vulkan rendering backends with ability to switch them out at will. The engine executes a RTS-battle scenarion until stopped.

Project uses CMake. You'll need VulkanSDK, GLEW, GLM, GLFW, OpenAL, freeglut and Intel's TBB. Tested on Windows, should be runnable on Linux(no Mac support because of Vulkan) too.

Intel TBB can be fast found using TBB_ROOT_DIR entry.

Tank model was found on Unity Asset Store (https://www.assetstore.unity3d.com/en/#!/content/46209)
Audio files were found here:
 * https://www.freesound.org/people/joshuaempyre/sounds/251461/
 * http://soundbible.com/1326-Tank-Firing.html
 * http://soundbible.com/1467-Grenade-Explosion.html

Tasks
=====


### OpenGL
* Animations
* Add debug drawing (sphere for now)

### Vulkan
* Figure out why can't reuse the swapchain when recreating it on resize
* Terrain doesn't render properly
* Animations
* Add debug drawing (sphere for now)