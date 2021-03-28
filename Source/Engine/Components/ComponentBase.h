#pragma once

class GameObject;
class Serializer;

class ComponentBase
{
public:
	ComponentBase() : myOwner(nullptr) {}
	virtual ~ComponentBase() {};

	virtual void Init(GameObject* anOwner) { myOwner = anOwner; };

	GameObject* GetOwner() { return myOwner; }

	virtual std::string_view GetName() const = 0;
	virtual void Serialize(Serializer&) { ASSERT_STR(false, "NYI"); };

protected:
	GameObject* myOwner;
};

class ComponentRegister
{
	using CreateFunc = ComponentBase*(*)();
public:
	static ComponentRegister& Get()
	{
		static ComponentRegister registrator;
		return registrator;
	}

	void Register(std::string_view aName, CreateFunc aFunc)
	{
		ASSERT_STR(myCreateFuncs.find(aName) == myCreateFuncs.end(), "%s already registered!", aName.data());
		myCreateFuncs.insert({ aName, aFunc });
	}

	ComponentBase* Create(std::string_view aName) const
	{
		auto funcIter = myCreateFuncs.find(aName);
		ASSERT_STR(funcIter != myCreateFuncs.end(), "Missing Creation callback for %s", aName.data());
		return funcIter->second();
	}

private:
	std::unordered_map<std::string_view, CreateFunc> myCreateFuncs;
};

// Optional helper to cause component to implement 
// auto-registration with ComponentRegister for serialization. 
// Requires:
// * TComp must define constexpr static kName
// * TComp must override Serialize(Serialzier&)
template<class TComp, class TBase = ComponentBase>
class SerializableComponent : public TBase
{
public:
	// this is needed to get past ODR and cause implicit template instantiation
	~SerializableComponent() { ourMyRegistrar; } 
	std::string_view GetName() const override { return TComp::kName; }

private:
	static bool ourMyRegistrar;
};

template<class TComp, class TBase>
bool SerializableComponent<TComp, TBase>::ourMyRegistrar = [] {
	ComponentRegister::Get().Register(TComp::kName, [] { 
		return static_cast<ComponentBase*>(new TComp());
	});
	return true;
} ();