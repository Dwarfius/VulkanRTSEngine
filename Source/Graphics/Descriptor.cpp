#include "Precomp.h"
#include "Descriptor.h"

#include <Core/Resources/Serializer.h>

void Descriptor::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myUniformAdapter", myUniformAdapter);

	if (Serializer::Scope membersScope = aSerializer.SerializeArray("myEntries", myEntries))
	{
		for (size_t i = 0; i < myEntries.size(); i++)
		{
			if (Serializer::Scope entryScope = aSerializer.SerializeObject(i))
			{
				aSerializer.SerializeEnum("myUniformType", myEntries[i].myUniformType);
				aSerializer.Serialize("myArraySize", myEntries[i].myArraySize);
			}
		}
	}
	RecomputeSize();
}