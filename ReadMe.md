Info
====

This is a mutltithreaded engine project indended as a research in to Vulkan API. The engine features a OpenGL and Vulkan rendering backends with ability to switch them out at will. The engine executes a RTS-battle scenarion until stopped.

Project uses CMake(3.13). I'm moving as many dependencies as I can to be auto-fetched by CMake's Fetch Content. 
* CMake will fetch and configure for you:
	* [JSON for Modern C++ by Niels Lohmann](https://github.com/nlohmann/json)
	* [GLM](https://glm.g-truc.net)
	* [GLEW](http://glew.sourceforge.net/) will be grabbed from a [cmake fork](https://github.com/Perlmint/glew-cmake)
	* [Intel's TBB](https://github.com/01org/tbb)
	* [GLFW](https://www.glfw.org/)
* For now you'll need to manually grab:
	* [VulkanSDK by LunarG](https://www.lunarg.com/)
	* [OpenAL](https://www.openal.org/)
	* [freealut](https://github.com/vancegroup/freealut)
	* [Bullet3](https://github.com/bulletphysics/bullet3)

Intel TBB can be fast found using TBB_ROOT_DIR entry.

Tank model was found on Unity Asset Store (https://www.assetstore.unity3d.com/en/#!/content/46209)
Audio files were found here:
 * https://www.freesound.org/people/joshuaempyre/sounds/251461/
 * http://soundbible.com/1326-Tank-Firing.html
 * http://soundbible.com/1467-Grenade-Explosion.html

Test Video
==========

A minigame implemented using the engine, where the player has to drive around the tank, shoot down enemy tanks while avoiding getting touched by others. If caught, the game ends, and the amount of destroyed tanks is the final score.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=yWnIchIsI7E" target="_blank"><img src="http://img.youtube.com/vi/yWnIchIsI7E/0.jpg" alt="Youtube image" width="240" height="180" border="10" /></a>

Stress Test
===========

A modification of the game logic to stress test the game engine. The engine constantly spawns new tanks for the 2 teams, orders them to move to the other side. The tanks shoot projectiles which destroy enemy vehicles on contact. As visible from the video, rendering artifacts are present - the TBB implementation needs to be improved to avoid this issue. The game scenario simulates about 2600 objects total for a duration of 1 minute. The spawn rate gradually increases from start, and continues to increase throught the test. The objects stabilize at 2600 mark due to nature of the test - it doesn't matter how many are created, they destroy each other as fast.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=7t4nZ0Hbtok" target="_blank"><img src="http://img.youtube.com/vi/7t4nZ0Hbtok/0.jpg" alt="Youtube image" width="240" height="180" border="10" /></a>

License
===========

* The project relies on [JSON for Modern C++ by Niels Lohmann](https://github.com/nlohmann/json) which is licensed under the MIT license (see below). Copyright © 2013-2018 [Niels Lohmann](http://nlohmann.me/)
* The project relies on [Intel's TBB](https://github.com/01org/tbb) which is licensed under the Apache License 2.0 (see [Licenses](Copying.md)). Copyright © [Intel](https://www.threadingbuildingblocks.org/)
* The project relies on [freealut](https://github.com/vancegroup/freealut) which is licensed under LGPL (see [Licenses](Copying.md)). Copyright © [Dr. Judy M. Vance](http://www.me.iastate.edu/jmvance/)
* The project relies on [Bullet3](https://github.com/bulletphysics/bullet3) which is licensed under permissive zlib license (see [Licenses](Copying.md)). Copyright © [Bullet Physics Dev Team](https://github.com/bulletphysics), [erwin.coumans@gmail.com](erwin.coumans@gmail.com)
* The project relies on [GLFW](https://www.glfw.org) which is licensed under the zlib/libpng license (see [Licenses](Copying.md)). Copyright © 2002-2006 [Marcus Geelnard](https://www.glfw.org/license.html), Copyright © 2006-2018 [Camilla Löwy](https://www.glfw.org/license.html)
* The project relies on [GLM](https://glm.g-truc.net) which is licensed under The Happy Bunny License and MIT License (see [Licenses](Copying.md)). Copyright © [G-Truc Creation](https://github.com/g-truc)
* The project relies on [GLEW](http://glew.sourceforge.net) which is licensed under a modified BSD license (see [Licenses](Copying.md)). 
	* Copyright © 2008-2016, Nigel Stewart <nigels[]users sourceforge net>
	* Copyright © 2002-2008, Milan Ikits <milan ikits[]ieee org>
	* Copyright © 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
	* Copyright © 2002, Lev Povalahev
* This project relies on [LunarG's VulkanSDK](https://www.lunarg.com/vulkan-sdk/) which is licensed under the Apache License 2.0(see [Licenses](Copying.md)). 
	* Copyright © 2015-2017 LunarG, Inc.
	* Copyright © 2015-2017 Valve Corporation

[MIT License](http://opensource.org/licenses/MIT): 
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.