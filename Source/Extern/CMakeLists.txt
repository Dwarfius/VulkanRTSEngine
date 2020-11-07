# by Daniel Prihodko
cmake_minimum_required (VERSION 3.16)

set(CMAKE_FOLDER Extern/STB_Image)
add_subdirectory(STB_Image)

set(CMAKE_FOLDER Extern/TinyObjLoader)
add_subdirectory(TinyObjLoader)

set(CMAKE_FOLDER Extern/ImGUI)
add_subdirectory(ImGUI)