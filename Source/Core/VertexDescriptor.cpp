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
	size_t memberCount = myMemberCount;
	if (Serializer::Scope membersScope = aSerializer.SerializeArray("myMembers", memberCount))
	{
		myMemberCount = static_cast<uint8_t>(memberCount);
		for (size_t i = 0; i < myMemberCount; i++)
		{
			if (Serializer::Scope memberScope = aSerializer.SerializeObject(i))
			{
				myMembers[i].Serialize(aSerializer);
			}
		}
	}
}