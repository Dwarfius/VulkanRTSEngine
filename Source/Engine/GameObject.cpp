#include "Common.h"
#include "GameObject.h"
#include "Game.h"
#include "Graphics.h"
#include "Transform.h"

#include "Components/ComponentBase.h"
#include "Components/Renderer.h"

GameObject::GameObject(glm::vec3 aPos, glm::vec3 aRot, glm::vec3 aScale)
	: myUID(UID::Create())
	, myTransf(aPos, aRot, aScale)
	, myCurrentMat()
	, myRenderer(nullptr)
	, myIsDead(false)
	, myIndex(numeric_limits<size_t>::max())
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
}

void GameObject::Update(float aDeltaTime)
{
	for (int i = 0; i < myComponents.size(); i++)
	{
		myComponents[i]->Update(aDeltaTime);
	}

	myCurrentMat = myTransf.GetModelMatrix();

	if (myRenderer)
	{
		Shader::UniformValue val;
		val.myMatrix = myCurrentMat;
		myUniforms["Model"] = val;
	}
}

float GameObject::GetRadius() const
{
	if (!myRenderer)
	{
		return 0;
	}

	const glm::vec3 scale = myTransf.GetScale();
	const float maxScale = max({ scale.x, scale.y, scale.z });
	const float radius = Game::GetInstance()->GetGraphics()->GetModelRadius(myRenderer->GetModel());
	return maxScale * radius;
}

void GameObject::AddComponent(ComponentBase* aComponent)
{
	aComponent->Init(this);
	myComponents.push_back(aComponent);
	Renderer* newRenderer = dynamic_cast<Renderer*>(aComponent);
	if (newRenderer && !myRenderer)
	{
		myRenderer = newRenderer;
		// TODO: look into making this optional per model
		myTransf.SetCenter(Game::GetInstance()->GetGraphics()->GetModelCenter(myRenderer->GetModel()));
	}
	else if (newRenderer && myRenderer)
	{
		printf("[Warning] Attempting to attach a renderer to a component with a renderer, ignoring\n");
	}
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