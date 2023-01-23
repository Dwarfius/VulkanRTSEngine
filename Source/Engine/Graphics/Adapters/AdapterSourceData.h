#pragma once

#include <Graphics/UniformAdapterRegister.h>

class GameObject;
class VisualObject;
class Terrain;

struct UniformAdapterSource : CameraAdapterSourceData
{
	const GameObject* myGO; // should not be nullptr if Adapter relies on it
	const VisualObject& myVO;
};