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
}