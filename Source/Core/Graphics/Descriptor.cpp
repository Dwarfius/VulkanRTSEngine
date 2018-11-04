#include "Precomp.h"
#include "Descriptor.h"

Descriptor::Descriptor(const string& anAdapterName)
	: myTotalSize(0)
	, myUniformAdapter(anAdapterName)
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
	for (size_t i = 1; i <= size; i++)
	{
		size_t uniformSize = 0;
		switch (myTypes[i - 1])
		{
		case UniformType::Float:
		case UniformType::Int:
			uniformSize = sizeof(int);
			break;
		case UniformType::Vec2:
			uniformSize = sizeof(glm::vec2);
			break;
		case UniformType::Vec3:
			uniformSize = sizeof(glm::vec3);
			break;
		case UniformType::Vec4:
			uniformSize = sizeof(glm::vec4);
			break;
		case UniformType::Mat4:
			uniformSize = sizeof(glm::mat4);
			break;
		default:
			ASSERT_STR(false, "Unrecognized Uniform Type found!");
		}
		myOffsets[i - 1] = myTotalSize;
		myTotalSize += uniformSize;
	}
}