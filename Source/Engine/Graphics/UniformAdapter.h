#pragma once

class GameObject;
class VisualObject;
class UniformBlock;
class Camera;

// A utility class which knows what data to grab from the game objects
// to pass to rendering. A temporary helper, until a more automatic flow is
// implemented.
// This basic adapter only provides Model and MVP matrices as uniforms.
// To provide different versions of the adapter, just inherit the adapter
// and override the FillUniformBlock.
class UniformAdapter
{
public:
	// Both aGO and aVO can be used to access information needed to render the object
	UniformAdapter(const GameObject& aGO, const VisualObject& aVO);

	virtual void FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const;

protected:
	const GameObject& myGO;
	const VisualObject& myVO;
};