#pragma once

#include <Graphics/UniformAdapter.h>

class GameObject;
class VisualObject;

struct UniformAdapterSource : UniformAdapter::SourceData
{
	const GameObject& myGO;
	const VisualObject& myVO;
};