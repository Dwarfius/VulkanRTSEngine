#include "Precomp.h"
#include "Descriptor.h"

char* Descriptor::GetPtr(uint32_t aSlot, size_t anArrayIndex, char* aBasePtr) const
{
	const Entry& entry = myEntries[aSlot];
	size_t slotSize = 0;
	size_t ignored;
	GetSizeAndAlignment(aSlot, slotSize, ignored);
	return aBasePtr + entry.myByteOffset + anArrayIndex * slotSize;
}

static constexpr Descriptor kDesc{
	{Descriptor::UniformType::Int}
};
static_assert(kDesc.GetUniformCount() == 1);