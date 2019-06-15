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
			shared_ptr<UniformAdapter>(*)(const GameObject&, const VisualObject&); \
		static constexpr CreationMethod GetCreationMethod() \
		{ \
			return [](const GameObject& aGO, const VisualObject& aVO) \
				-> shared_ptr<UniformAdapter> \
			{ \
				return make_shared<Type>(aGO, aVO); \
			}; \
		}

// A utility class which knows what data to grab from the game objects
// to pass to rendering. A temporary helper, until a more automatic flow is
// implemented.
// This basic adapter only provides Model and MVP matrices as uniforms.
// To provide different versions of the adapter, just inherit the adapter
// and override the FillUniformBlock.
class UniformAdapter
{
	DECLARE_REGISTER(UniformAdapter);
public:
	// Both aGO and aVO can be used to access information needed to render the object
	UniformAdapter(const GameObject& aGO, const VisualObject& aVO);

	virtual void FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const;

protected:
	const GameObject& myGO;
	const VisualObject& myVO;
};