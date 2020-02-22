#pragma once

class GameObject;
class VisualObject;
class UniformBlock;
class Camera;

// Use this macro to enable registering a uniform adapter 
// specialization with the adapter register
#define DECLARE_REGISTER(Type) \
	private: \
		friend class UniformAdapterRegister; \
		static constexpr const char* GetName() { return #Type; } \
		using CreationMethod = \
			std::shared_ptr<UniformAdapter>(*)(const GameObject&, const VisualObject&); \
		static constexpr CreationMethod GetCreationMethod() \
		{ \
			return [](const GameObject& aGO, const VisualObject& aVO) \
				-> std::shared_ptr<UniformAdapter> \
			{ \
				return std::make_shared<Type>(aGO, aVO); \
			}; \
		}

// A utility class which knows what data to grab from the game objects
// to pass to rendering.
class UniformAdapter
{
public:
	// Both aGO and aVO can be used to access information needed to render the object
	UniformAdapter(const GameObject& aGO, const VisualObject& aVO)
		: myGO(aGO)
		, myVO(aVO)
	{
	}
	virtual void FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const = 0;

protected:
	const GameObject& myGO;
	const VisualObject& myVO;
};