#pragma once

#include <Core/DataEnum.h>
#include <Graphics/UniformAdapterRegister.h>

class Descriptor;
template<class T> class Handle;

class IPipeline
{
public:
	DATA_ENUM(Type, char,
		Graphics,
		Compute
	);

	virtual size_t GetDescriptorCount() const = 0;
	virtual const Descriptor& GetDescriptor(size_t) const = 0;
	virtual UniformAdapterRegister::FillUBCallback GetAdapter(size_t) const = 0;
};