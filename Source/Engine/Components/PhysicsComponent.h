#pragma once

#include "ComponentBase.h"

class PhysicsEntity;
class PhysicsWorld;
class PhysicsShapeBase;

// Component that handles the life-cycle of a physics entity
class PhysicsComponent : public SerializableComponent<PhysicsComponent>
{
public:
	constexpr static std::string_view kName = "PhysicsComponent";

	PhysicsComponent();
	virtual ~PhysicsComponent();

	bool IsInitialized() const { return myEntity != nullptr; }
	PhysicsEntity& GetPhysicsEntity() const { return *myEntity; }

	void SetOrigin(const glm::vec3& anOrigin) { myOrigin = anOrigin; }

	// A mass of 0 denotes a static phys entity!
	void CreatePhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape);
	void DeletePhysicsEntity();

	bool IsInWorld() const;
	void RequestAddToWorld(PhysicsWorld& aWorld) const;

	void Serialize(Serializer& aSerializer) final;

private:
	// TODO: rewrite using unique_ptr
	// we own a phys entity, until we don't - see dtor
	PhysicsEntity* myEntity;
	glm::vec3 myOrigin;
};