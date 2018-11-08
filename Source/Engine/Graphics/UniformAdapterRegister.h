#pragma once

class UniformAdapter;
class GameObject;
class VisualObject;

// A Singleton class used for tracking all the uniform adapters
// that can be used to bridge the game state and rendering
class UniformAdapterRegister
{
	using CreationMethod = shared_ptr<UniformAdapter>(*)(const GameObject&, const VisualObject&);
public:
	static UniformAdapterRegister* GetInstance();

	shared_ptr<UniformAdapter> GetAdapter
		(const string& aName, const GameObject& aGO, const VisualObject& aVO) const;
	
private:
	static UniformAdapterRegister* myInstance;
	unordered_map<string, CreationMethod> myCreationMethods;

	void RegisterTypes();

	// Utility method that simplifies registering of adapters
	template<class Type>
	void Register()
	{
		string name = Type::GetName();
		const CreationMethod& creationMethod = Type::GetCreationMethod();
		pair<string, CreationMethod> newPair = make_pair(name, creationMethod);
		myCreationMethods.insert(newPair);
	}
};