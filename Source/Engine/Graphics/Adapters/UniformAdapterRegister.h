#pragma once

class UniformAdapter;
class GameObject;
class VisualObject;

// A Singleton class used for tracking all the uniform adapters
// that can be used to bridge the game state and rendering
class UniformAdapterRegister
{
	using CreationMethod = std::shared_ptr<UniformAdapter>(*)(const GameObject&, const VisualObject&);
public:
	static UniformAdapterRegister* GetInstance();

	std::shared_ptr<UniformAdapter> GetAdapter
		(const std::string& aName, const GameObject& aGO, const VisualObject& aVO) const;

	// Utility method that simplifies registering of adapters
	template<class Type>
	void Register()
	{
		std::string name = Type::GetName();
		const CreationMethod& creationMethod = Type::GetCreationMethod();
		std::pair<std::string, CreationMethod> newPair = std::make_pair(name, creationMethod);
		myCreationMethods.insert(newPair);
	}
	
private:
	static UniformAdapterRegister* myInstance;
	std::unordered_map<std::string, CreationMethod> myCreationMethods;

	void RegisterTypes();
};