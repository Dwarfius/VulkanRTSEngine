#include "Precomp.h"
#include "Descriptor.h"

#include <Core/Resources/Serializer.h>

Descriptor::Descriptor()
	: myTotalSize(0)
{
}

Descriptor::Descriptor(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myTotalSize(0)
{

}

void Descriptor::SetUniformType(uint32_t aSlot, UniformType aType)
{
	if (aSlot >= myTypes.size())
	{
		myTypes.resize(aSlot + 1);
	}
	myTypes[aSlot] = aType;
}

void Descriptor::RecomputeSize()
{
	const size_t size = myTypes.size();
	myOffsets.resize(size);

	myTotalSize = 0;
	for (size_t i = 0; i < size; i++)
	{
		size_t basicAlignment;
		size_t elemSize;
		switch (myTypes[i])
		{
		case UniformType::Float:
		case UniformType::Int:
			basicAlignment = sizeof(int);
			elemSize = sizeof(int);
			break;
		case UniformType::Vec2:
			basicAlignment = sizeof(glm::vec2);
			elemSize = sizeof(glm::vec2);
			break;
		case UniformType::Vec3:
			// std140 - https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf#page=159
			// If the member is a three-component vector with components consuming N
			// basic machine units, the base alignment is 4N
			// Despite that, glsl compiler can merge the next single unit
			// to this 3, to avoid wasting MUs.
			if (i + 1 < size
				&& (myTypes[i + 1] == UniformType::Float
					|| myTypes[i + 1] == UniformType::Int))
			{
				basicAlignment = sizeof(glm::vec4);
				elemSize = sizeof(glm::vec3);
			}
			else
			{
				basicAlignment = sizeof(glm::vec4);
				elemSize = sizeof(glm::vec4);
			}
			break;
		case UniformType::Vec4:
			basicAlignment = sizeof(glm::vec4);
			elemSize = sizeof(glm::vec4);
			break;
		case UniformType::Mat4:
			basicAlignment = sizeof(glm::vec4);
			elemSize = sizeof(glm::mat4);
			break;
		default:
			ASSERT_STR(false, "Unrecognized Uniform Type found!");
		}
		// myTotalSize has the aligned offset from the previous iter
		// plus the size of the new element.
		// calculate the adjusted size (with padding), so that we can
		// retain alignment required for new element
		size_t remainder = myTotalSize % basicAlignment;
		if (remainder)
		{
			myTotalSize -= remainder;
			myTotalSize += basicAlignment;
		}
		
		ASSERT_STR(myTotalSize < std::numeric_limits<uint32_t>::max(), "Descriptor size exceeding allowed!");
		myOffsets[i] = static_cast<uint32_t>(myTotalSize);
		myTotalSize += elemSize;
	}

	// since it's a struct, per std140 rules it has to be 16-aligned
	constexpr size_t kStructAlignment = 16;
	size_t remainder = myTotalSize % kStructAlignment;
	if (remainder)
	{
		myTotalSize -= remainder;
		myTotalSize += kStructAlignment;
	}
}

size_t Descriptor::GetSlotSize(uint32_t aSlot) const
{
	switch (myTypes[aSlot])
	{
	case UniformType::Float:
	case UniformType::Int:
		return sizeof(int);
		break;
	case UniformType::Vec2:
		return sizeof(glm::vec2);
		break;
	case UniformType::Vec3:
		return sizeof(glm::vec3);
		break;
	case UniformType::Vec4:
		return sizeof(glm::vec4);
		break;
	case UniformType::Mat4:
		return sizeof(glm::mat4);
		break;
	default:
		ASSERT_STR(false, "Unrecognized Uniform Type found!");
		return 0;
	}
}

void Descriptor::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("adapter", myUniformAdapter);

	if (Serializer::Scope membersScope = aSerializer.SerializeArray("members", myTypes))
	{
		for (size_t i = 0; i < myTypes.size(); i++)
		{
			aSerializer.Serialize(i, myTypes[i]);
		}
	}
	RecomputeSize();
}