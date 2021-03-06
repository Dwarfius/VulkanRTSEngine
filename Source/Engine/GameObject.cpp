#include "Precomp.h"
#include "GameObject.h"
#include "Game.h"
#include "VisualObject.h"
#include "Components/ComponentBase.h"

#include <Core/Transform.h>

#include <Graphics/Graphics.h>
#include <memory_resource>

GameObject::GameObject(const Transform& aTransform)
	: myUID(UID::Create())
	, myWorldTransf(aTransform)
	, myLocalTransf(aTransform)
	, myVisualObject(nullptr)
	, myIsDead(false)
	, myCenter(0)
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
		myVisualObject->SetTransform(myWorldTransf);
	}
}

void GameObject::SetLocalTransform(const Transform& aTransf, bool aMoveChildren /* = true */)
{
	AssertLock lock(myPhysMutex);
	if (aTransf != myLocalTransf)
	{
		myLocalTransf = aTransf;

		Transform parentTransf;
		if (myParent.IsValid())
		{
			parentTransf = myParent->GetWorldTransform();
		}
		myWorldTransf = parentTransf * myLocalTransf;

		if (aMoveChildren)
		{
			UpdateHierarchy();
		}
	}
}

void GameObject::SetWorldTransform(const Transform& aTransf, bool aMoveChildren /* = true */)
{
	AssertLock lock(myPhysMutex);
	if (aTransf != myWorldTransf)
	{
		myWorldTransf = aTransf;

		Transform parentTransf;
		if (myParent.IsValid())
		{
			parentTransf = myParent->GetWorldTransform();
		}
		myLocalTransf = parentTransf * myWorldTransf.GetInverted();

		if (aMoveChildren)
		{
			UpdateHierarchy();
		}
	}
}

void GameObject::Die()
{
	myIsDead = true;
	Game::GetInstance()->RemoveGameObject(this);
}

void GameObject::SetParent(Handle<GameObject>& aParent, bool aKeepWorldTransf /* = true */)
{
	ASSERT_STR(aParent.IsValid(), "Trying to use invalid game object!");
	myParent = aParent;
	if (aKeepWorldTransf)
	{
		// force recalculation of world transf
		myWorldTransf = {};
		SetWorldTransform(myWorldTransf);
	}
	else
	{
		// force recalculation of local transf
		myLocalTransf = {};
		SetLocalTransform(myWorldTransf);
	}
}

void GameObject::DetachFromParent()
{
	ASSERT_STR(myParent.IsValid(), "Object doesn't have a parent!");
	myParent->DetachChild(this);
	myParent = Handle<GameObject>();
}

void GameObject::AddChild(Handle<GameObject>& aChild, bool aKeepChildWorldTransf /* = true */)
{
	ASSERT_STR(aChild.IsValid(), "Trying to assign an invalid game object!");
	Handle<GameObject> thisHandle = this;
	aChild->SetParent(thisHandle, aKeepChildWorldTransf);
	myChildren.push_back(aChild);
}

void GameObject::DetachChild(size_t aInd)
{
	ASSERT_STR(aInd < myChildren.size(), "No child with index %llu!", aInd);
	myChildren[aInd]->myParent = Handle<GameObject>();
	myChildren[aInd]->SetLocalTransform(myChildren[aInd]->GetWorldTransform());
	myChildren.erase(myChildren.begin() + aInd);
}

void GameObject::DetachChild(const Handle<GameObject>& aGO)
{
	const auto iter = std::find(myChildren.begin(), myChildren.end(), aGO);
	ASSERT_STR(iter != myChildren.end(), "Invalid child!");
	const size_t index = static_cast<size_t>(iter - myChildren.begin());
	DetachChild(index);
}

void GameObject::SetPhysTransform(const glm::mat4& aTransf)
{
	AssertLock lock(myPhysMutex);
	// because Bullet doesn't support scaled transforms, 
	// we can safely split the mat4 into mat3 rot and pos
	Transform newTransf;
	newTransf.SetPos(aTransf[3]);
	newTransf.SetRotation(glm::quat_cast(aTransf));
	if (newTransf != myWorldTransf)
	{
		myWorldTransf = newTransf;

		Transform parentTransf;
		if (myParent.IsValid())
		{
			parentTransf = myParent->GetWorldTransform();
		}
		myLocalTransf = parentTransf * myWorldTransf.GetInverted();

		UpdateHierarchy();
	}
}

void GameObject::GetPhysTransform(glm::mat4& aTransf) const
{
	aTransf = myWorldTransf.GetMatrix();
}

void GameObject::UpdateHierarchy()
{
	// Expectation that local transform is up to date!
	constexpr size_t kGOMemorySize = 128;
	GameObject* ptrMemory[kGOMemorySize];
	std::pmr::monotonic_buffer_resource stackRes(ptrMemory, sizeof(ptrMemory));
	std::pmr::vector<GameObject*> dirtyGOs(&stackRes);
	for (Handle<GameObject>& child : myChildren)
	{
		dirtyGOs.push_back(child.Get());
	}

	while (dirtyGOs.size())
	{
		GameObject* go = dirtyGOs.back();
		dirtyGOs.pop_back();

		go->myWorldTransf = go->GetParent()->GetWorldTransform() * go->myLocalTransf;
		for (Handle<GameObject>& child : go->myChildren)
		{
			dirtyGOs.push_back(child.Get());
		}
	}
}