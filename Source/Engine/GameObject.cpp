#include "Precomp.h"
#include "GameObject.h"
#include "Game.h"
#include "VisualObject.h"
#include "Components/ComponentBase.h"

#include <Core/Transform.h>
#include <Core/Resources/Serializer.h>

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

GameObject::GameObject(Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myUID(UID::Create())
	, myWorldTransf(Transform())
	, myLocalTransf(Transform())
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

void GameObject::Serialize(Serializer& aSerializer)
{
	// Checking if we're deserializing a child GameObject
	// If so - we can abort read serialization if child is the only holder of
	// parent handle
	const bool isReading = aSerializer.IsReading();
	if (isReading && myParent.IsValid() && myParent.IsLastHandle())
	{
		// it is - break the circular reference and early out
		// this will avoid zombie objects,
		// save time and prevent further hierarchy deserialization
		myParent = Handle<GameObject>();
		return;
	}

	if (Serializer::Scope uidScope = aSerializer.SerializeObject("myUID"))
	{
		myUID.Serialize(aSerializer);
	}
	
	// we can reconstruct world from local, so only serializing local
	if (Serializer::Scope localTransfScope = aSerializer.SerializeObject("myLocalTransf"))
	{
		myLocalTransf.Serialize(aSerializer);
		if (isReading)
		{
			// because we're serializing hierarchy top to bottom,
			// parent GO can set the myParent early
			if(myParent.IsValid())
			{
				myWorldTransf = myParent->GetWorldTransform() * myLocalTransf;
			}
			else
			{
				myWorldTransf = myLocalTransf;
			}
		}
	}

	aSerializer.Serialize("myCenter", myCenter);

	size_t compsCount = myComponents.size();
	if (Serializer::Scope compsScope = aSerializer.SerializeArray("myComponents", compsCount))
	{
		if (isReading)
		{
			myComponents.resize(compsCount);
		}

		for (size_t i = 0; i < compsCount; i++)
		{
			if (Serializer::Scope compTypeScope = aSerializer.SerializeObject(i))
			{
				if (isReading)
				{
					std::string compType;
					std::string_view oldCompName;
					if (myComponents[i])
					{
						compType = myComponents[i]->GetName();
						oldCompName = myComponents[i]->GetName();
					}
					aSerializer.Serialize("myCompType", compType);
					if (!compType.empty())
					{
						if (compType != oldCompName)
						{
							if (myComponents[i])
							{
								delete myComponents[i];
							}
							myComponents[i] = ComponentRegister::Get().Create(compType);
							myComponents[i]->Init(this);
						}
					}
					else
					{
						myComponents[i] = nullptr;
					}
				}
				else
				{
					std::string_view compTypeView = myComponents[i]->GetName();
					std::string compType(compTypeView.data(), compTypeView.size());
					aSerializer.Serialize("myCompType", compType);
				}

				if (Serializer::Scope compScope = aSerializer.SerializeObject("myCompData"))
				{
					myComponents[i]->Serialize(aSerializer);
				}
			}
		}
	}

	// We're skipping parent serialization as it doesn't make much sense in
	// saving who's the parent of current object, as we serialize hierarchy top to bottom
	if (Serializer::Scope childrenScope = aSerializer.SerializeArray("myChildren", myChildren))
	{
		for (size_t i = 0; i < myChildren.size(); i++)
		{
			aSerializer.Serialize(i, myChildren[i]);
			// This is potentially dangerous - circular dep!
			// To mitigate, children's serialization starts with a check if they own
			// the last handle
			myChildren[i]->myParent = this; 
		}
	}
}