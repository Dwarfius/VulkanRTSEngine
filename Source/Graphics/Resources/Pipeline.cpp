#include "Precomp.h"
#include "Pipeline.h"

#include "../Graphics.h"
#include "../UniformAdapterRegister.h"
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

void Pipeline::AddAdapter(std::string_view anAdapter)
{
	myAdapters.push_back(&UniformAdapterRegister::GetInstance().GetAdapter(anAdapter));
}

void Pipeline::Serialize(Serializer& aSerializer)
{
	aSerializer.SerializeEnum("myType", myType);
	ASSERT_STR(myType == IPipeline::Type::Graphics, "Compute pipeline needs implementing!");

	if (Serializer::Scope shadersScope = aSerializer.SerializeArray("myShaders", myShaders))
	{
		for (size_t i = 0; i < myShaders.size(); i++)
		{
			aSerializer.Serialize(i, myShaders[i]);
		}
	}

	if(Serializer::Scope adaptersScope = aSerializer.SerializeArray("myAdapters", myAdapters))
	{
		for (size_t i = 0; i < myAdapters.size(); i++)
		{
			std::string adapterName = myAdapters[i] ? 
				std::string{ myAdapters[i]->GetName().data(), myAdapters[i]->GetName().size() }
				: std::string();
			aSerializer.Serialize(i, adapterName);
			if (aSerializer.IsReading())
			{
				myAdapters[i] = &UniformAdapterRegister::GetInstance().GetAdapter(adapterName);
			}
		}
	}
}