#pragma once

class UniformBlock;
class Camera;
class Graphics;

// Use this macro to enable registering a uniform adapter 
// specialization with the adapter register
#define DECLARE_REGISTER(Type) \
	private: \
		friend class UniformAdapterRegister; \
		static constexpr const char* GetName() { return #Type; } \

// A utility class which knows what data to grab from the game objects
// to pass to rendering. Must not have any state because we only ever use
// 1 instance of adapter in every case (can be shared across threads)
class UniformAdapter
{
public:
	struct SourceData 
	{
		const Graphics& myGraphics;
		const Camera& myCam;
	};

	virtual void FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const = 0;
};
static_assert(sizeof(UniformAdapter) == sizeof(void*), "Must be a state-less(except vtable) type!");