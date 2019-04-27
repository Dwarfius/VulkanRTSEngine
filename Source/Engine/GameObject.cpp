#include "Precomp.h"
#include "GameObject.h"
#include "Game.h"
#include "VisualObject.h"

#include <Core/Graphics/Graphics.h>
#include <Core/Transform.h>
#include "Components/ComponentBase.h"

GameObject::GameObject(glm::vec3 aPos, glm::vec3 aRot, glm::vec3 aScale)
	: myUID(UID::Create())
	, myTransf(aPos, aRot, aScale)
	, myVisualObject(nullptr)
	, myIsDead(false)
{
}

GameObject::~GameObject()
{
	ASSERT_STR(Game::ourGODeleteEnabled, "GameObject got deleted outside cleanup stage!");

	for (ComponentBase* comp : myComponents)
	{
		delete comp;
	}

	if (myVisualObject)
	{
		delete myVisualObject;
	}
}

void GameObject::Update(float aDeltaTime)
{
	// TODO: get rid of this in Update
	if (myVisualObject)
	{
		myVisualObject->SetTransform(myTransf);
	}
}

void GameObject::AddComponent(ComponentBase* aComponent)
{
	aComponent->Init(this);
	myComponents.push_back(aComponent);
}

ComponentBase* GameObject::GetComponent(int aType) const
{
	for (ComponentBase* comp : myComponents)
	{
		if (comp->GetComponentType() == aType)
		{
			return comp;
		}
	}
	return nullptr;
}

void GameObject::Die()
{
	myIsDead = true;
	Game::GetInstance()->RemoveGameObject(this);
}

void GameObject::SetTransformImpl(const glm::mat4& aTransf)
{
	// because Bullet doesn't support scaled transforms, 
	// we can safely split the mat4 into mat3 rot and pos
	myTransf.SetPos(aTransf[3]);
	myTransf.SetRotation(glm::quat_cast(aTransf));
}

void GameObject::GetTransformImpl(glm::mat4& aTranfs) const
{
	aTranfs = myTransf.GetMatrix();
}