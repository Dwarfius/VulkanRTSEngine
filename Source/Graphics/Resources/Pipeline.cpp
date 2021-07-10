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

void Pipeline::AddDescriptor(const Descriptor& aDescriptor)
{
	myDescriptors.push_back(aDescriptor);
	AddAdapter(aDescriptor);
}

void Pipeline::Serialize(Serializer& aSerializer)
{
	aSerializer.SerializeEnum("type", myType);
	ASSERT_STR(myType == IPipeline::Type::Graphics, "Compute pipeline needs implementing!");

	if (Serializer::Scope shadersScope = aSerializer.SerializeArray("shaders", myShaders))
	{
		for (size_t i = 0; i < myShaders.size(); i++)
		{
			aSerializer.Serialize(i, myShaders[i]);
		}
	}

	if(Serializer::Scope adaptersScope = aSerializer.SerializeArray("descriptors", myDescriptors))
	{
		for (size_t i = 0; i < myDescriptors.size(); i++)
		{
			if (Serializer::Scope objectScope = aSerializer.SerializeObject(i))
			{
				myDescriptors[i].Serialize(aSerializer);
			}
		}
	}
	
	AssignAdapters();
}

void Pipeline::AddAdapter(const Descriptor& aDescriptor)
{
	UniformAdapterRegister& adapterRegister = UniformAdapterRegister::GetInstance();
	const auto& adapter = adapterRegister.GetAdapter(aDescriptor.GetUniformAdapter());
	myAdapters.push_back(adapter);
}

void Pipeline::AssignAdapters()
{
	myAdapters.reserve(myDescriptors.size());
	for (const Descriptor& descriptor : myDescriptors)
	{
		AddAdapter(descriptor);
	}
}