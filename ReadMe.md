Info
====

This is a hobby project to practice developing a mutltithreaded engine. The engine features:
* an OpenGL rendering backend (supporting a simple feature set)
* a resource management system with PNG, JPG, GLTF and OBJ support
* an ImGUI integration
* an Animation system
* Physics driven by Bullet
* a standalone benchmarking executable using Google Benchmark

It is currently written using C++20 in Visual Studio 16.10.3 and tested on Windows.

Project uses CMake(3.17.3). I'm moving as many dependencies as I can to be auto-fetched by CMake's Fetch Content. 
* CMake will fetch and configure for you:
	* [JSON for Modern C++ by Niels Lohmann](https://github.com/nlohmann/json)
	* [GLM](https://glm.g-truc.net)
	* [GLEW](http://glew.sourceforge.net/) will be grabbed from a [cmake fork](https://github.com/Perlmint/glew-cmake)
	* [Intel's oneTBB](https://github.com/01org/tbb)
	* [GLFW](https://www.glfw.org/)
	* [Bullet3](https://github.com/bulletphysics/bullet3)
* It will optionally fetch:
	* [Google's Benchmark](https://github.com/google/benchmark) - for BenchTable

Tank model was found on Unity Asset Store (https://www.assetstore.unity3d.com/en/#!/content/46209)
Audio files were found here:
 * https://www.freesound.org/people/joshuaempyre/sounds/251461/
 * http://soundbible.com/1326-Tank-Firing.html
 * http://soundbible.com/1467-Grenade-Explosion.html

Obsolete - Test Video
==========

A minigame implemented using the engine, where the player has to drive around the tank, shoot down enemy tanks while avoiding getting touched by others. If caught, the game ends, and the amount of destroyed tanks is the final score.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=yWnIchIsI7E" target="_blank"><img src="http://img.youtube.com/vi/yWnIchIsI7E/0.jpg" alt="Youtube image" width="240" height="180" border="10" /></a>

Obsolete - Stress Test
===========

A modification of the game logic to stress test the game engine. The engine constantly spawns new tanks for the 2 teams, orders them to move to the other side. The tanks shoot projectiles which destroy enemy vehicles on contact. As visible from the video, rendering artifacts are present - the TBB implementation needs to be improved to avoid this issue. The game scenario simulates about 2600 objects total for a duration of 1 minute. The spawn rate gradually increases from start, and continues to increase throught the test. The objects stabilize at 2600 mark due to nature of the test - it doesn't matter how many are created, they destroy each other as fast.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=7t4nZ0Hbtok" target="_blank"><img src="http://img.youtube.com/vi/7t4nZ0Hbtok/0.jpg" alt="Youtube image" width="240" height="180" border="10" /></a>

License
===========

For license information, see LicenseNotes.md