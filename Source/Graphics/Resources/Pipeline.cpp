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

void Pipeline::AddDescriptor(Handle<Descriptor> aDescriptor)
{
	myDescriptors.push_back(aDescriptor);
	AddAdapter(aDescriptor);
}

void Pipeline::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("type", myType);
	ASSERT_STR(myType == IPipeline::Type::Graphics, "Compute pipeline type not supported!");

	aSerializer.Serialize("shaders", myShaders);
	aSerializer.Serialize("descriptors", myDescriptors);
	myAdapters.reserve(myDescriptors.size());
	for (Handle<Descriptor>& descriptor : myDescriptors)
	{
		AddAdapter(descriptor);
	}
}

void Pipeline::AddAdapter(Handle<Descriptor>& aDescriptor)
{
	// we can't immediately grab a descriptor, since it might not be loaded in yet
	// also, we don't control the order of descriptors loading, meaning
	// we can't guarantee that the adapter order will be correct - so we mitigate it
	// by collecting all loaded descriptors, and only populating the adapters then
	aDescriptor->ExecLambdaOnLoad([thisKeepAlive = Handle<Pipeline>(this)](const Resource* aRes) {
		Pipeline* thisPipeline = const_cast<Pipeline*>(thisKeepAlive.Get());
		uint8_t loadedDescriptors = 0;
		ASSERT_STR(thisPipeline->myDescriptors.size() < std::numeric_limits<decltype(loadedDescriptors)>::max(),
			"loadedDescriptors will overflow!");
		for (const Handle<Descriptor>& descriptor : thisPipeline->myDescriptors)
		{
			if (descriptor->GetState() == Resource::State::Ready)
			{
				loadedDescriptors++;
			}
		}
		if (loadedDescriptors == thisPipeline->myDescriptors.size())
		{
			thisPipeline->AssignAdapters();
		}
	});
	
}

void Pipeline::AssignAdapters()
{
	// by this point all descriptors should've loaded, so we can setup
	// adapters in the right order!
	for (const Handle<Descriptor>& descriptor : myDescriptors)
	{
		ASSERT_STR(descriptor->GetState() == Resource::State::Ready, "Descriptor must be loaded!");
		const auto& adapter = UniformAdapterRegister::GetInstance().GetAdapter(descriptor->GetUniformAdapter());
		myAdapters.push_back(adapter);
	}
}