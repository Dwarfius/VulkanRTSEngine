#include "Components\ComponentBase.h"

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