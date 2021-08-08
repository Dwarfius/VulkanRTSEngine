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

	PhysicsComponent() = default;
	~PhysicsComponent();

	bool IsInitialized() const { return myEntity != nullptr; }
	PhysicsEntity& GetPhysicsEntity() const { return *myEntity; }

	void SetOrigin(glm::vec3 anOrigin);
	glm::vec3 GetOrigin() const;

	// A mass of 0 denotes a static phys entity!
	void CreatePhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape, glm::vec3 anOrigin);
	void CreateOwnerlessPhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape, const glm::mat4& aTransf);
	void DeletePhysicsEntity();

	bool IsInWorld() const;
	void RequestAddToWorld(PhysicsWorld& aWorld) const;

	void Serialize(Serializer& aSerializer) final;

private:
	// TODO: rewrite using unique_ptr
	// we own a phys entity, until we don't - see dtor
	PhysicsEntity* myEntity = nullptr;
};