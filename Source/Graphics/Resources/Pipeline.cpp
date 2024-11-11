#include "Precomp.h"
#include "Pipeline.h"

#include "../Graphics.h"
#include "../UniformAdapterRegister.h"
#include <Core/Resources/Serializer.h>

Pipeline::Pipeline()
	: myType(IPipeline::Type::Graphics)
{
}

Pipeline::Pipeline(Resource::Id anId, std::string_view aPath)
	: Resource(anId, aPath)
	, myType(IPipeline::Type::Graphics)
{
}

void Pipeline::AddAdapter(std::string_view anAdapter)
{
	const UniformAdapter& adapter = UniformAdapterRegister::GetInstance().GetAdapter(anAdapter);
	if (adapter.IsGlobal())
	{
		myGlobalAdapters.push_back(&adapter);
	}
	else
	{
		myAdapters.push_back(&adapter);
	}
}

void Pipeline::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myType", myType);
	ASSERT_STR(myType == IPipeline::Type::Graphics, "Compute pipeline needs implementing!");

	aSerializer.Serialize("myShaders", myShaders);

	size_t adaptersSize = myAdapters.size();
	if (Serializer::ArrayScope adaptersScope{ aSerializer, "myAdapters", adaptersSize })
	{
		myAdapters.resize(adaptersSize);
		for (size_t i = 0; i < myAdapters.size(); i++)
		{
			std::string adapterName = myAdapters[i] ? 
				std::string{ myAdapters[i]->GetName().data(), myAdapters[i]->GetName().size() }
				: std::string();
			aSerializer.Serialize(Serializer::kArrayElem, adapterName);
			if (aSerializer.IsReading())
			{
				myAdapters[i] = &UniformAdapterRegister::GetInstance().GetAdapter(adapterName);
			}
		}
	}

	adaptersSize = myGlobalAdapters.size();
	if (Serializer::ArrayScope adaptersScope{ aSerializer, "myGlobalAdapters", adaptersSize })
	{
		myGlobalAdapters.resize(adaptersSize);
		for (size_t i = 0; i < myGlobalAdapters.size(); i++)
		{
			std::string adapterName = myGlobalAdapters[i] ?
				std::string{ myGlobalAdapters[i]->GetName().data(), myGlobalAdapters[i]->GetName().size() }
			: std::string();
			aSerializer.Serialize(Serializer::kArrayElem, adapterName);
			if (aSerializer.IsReading())
			{
				myGlobalAdapters[i] = &UniformAdapterRegister::GetInstance().GetAdapter(adapterName);
			}
		}
	}
}