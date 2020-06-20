#include "Precomp.h"
#include "Pipeline.h"

#include "../Graphics.h"
#include <Core/Resources/Serializer.h>

Pipeline::Pipeline()
	: myType(IPipeline::Type::Graphics)
{
}

Pipeline::Pipeline(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myType(IPipeline::Type::Graphics)
{
}

void Pipeline::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("type", myType);
	ASSERT_STR(myType == IPipeline::Type::Graphics, "Compute pipeline type not supported!");

	aSerializer.Serialize("shaders", myShaders);
	aSerializer.Serialize("descriptors", myDescriptors);
}