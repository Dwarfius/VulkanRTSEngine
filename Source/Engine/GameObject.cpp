#include "Precomp.h"
#include "GameObject.h"
#include "Game.h"
#include "Graphics/Graphics.h"
#include "Transform.h"
#include "VisualObject.h"

#include "Components/ComponentBase.h"

GameObject::GameObject(glm::vec3 aPos, glm::vec3 aRot, glm::vec3 aScale)
	: myUID(UID::Create())
	, myTransf(aPos, aRot, aScale)
	, myCurrentMat()
	, myVisualObject(nullptr)
	, myIsDead(false)
	, myCollisionsEnabled(true)
	, myCollidedWithTerrain(false)
{
}

GameObject::~GameObject()
{
	ASSERT_STR(Game::ourGODeleteEnabled, "GameObject got deleted outside cleanup stage!");

	for (ComponentBase* comp : myComponents)
	{
		comp->Destroy();
		delete comp;
	}

	if (myVisualObject)
	{
		delete myVisualObject;
	}
}

void GameObject::Update(float aDeltaTime)
{
	for (int i = 0; i < myComponents.size(); i++)
	{
		myComponents[i]->Update(aDeltaTime);
	}

	myTransf.UpdateModel();

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

void GameObject::CollidedWithTerrain()
{
	if (myCollidedWithTerrain)
	{
		return;
	}

	myCollidedWithTerrain = true;
	for (ComponentBase* base : myComponents)
	{
		base->OnCollideWithTerrain();
	}
}

void GameObject::CollidedWithGO(GameObject* aGo)
{
	if (myObjsCollidedWith.count(aGo))
	{
		return;
	}

	myObjsCollidedWith.insert(aGo);
	for (ComponentBase* base : myComponents)
	{
		base->OnCollideWithGO(aGo);
	}
}

void GameObject::PreCollision()
{
	myCollidedWithTerrain = false;
	myObjsCollidedWith.clear();
}

void GameObject::Die()
{
	myIsDead = true;
	Game::GetInstance()->RemoveGameObject(this);
}