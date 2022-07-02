#include "Precomp.h"
#include "VertexDescriptor.h"

#include "Resources/Serializer.h"

void VertexDescriptor::MemberDescriptor::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myType", myType);
	aSerializer.Serialize("myElemCount", myElemCount);
	aSerializer.Serialize("myOffset", myOffset);
}

void VertexDescriptor::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("mySize", mySize);
	aSerializer.Serialize("myMembers", myMembers);
}