#include "Components\ComponentBase.h"

const int ComponentBase::Type = 0;

void ComponentBase::Init(GameObject *o)
{
	owner = o;
}

void ComponentBase::Update(float deltaTime)
{

}

void ComponentBase::Destroy()
{

}

void ComponentBase::OnCollideWithTerrain()
{

}

void ComponentBase::OnCollideWithGO(GameObject *other)
{

}